#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "../include/file.h"
#include "../include/scanner.h"
#include "../include/monitor.h"
#include "../include/logger.h"
#include "../include/ipc.h"
#define ARCHIVO_PID "/tmp/minifilesync.pid"

// Rutas utilizadas por el programa
static char directorioBackup[MAX_RUTA] = "backup";
static char directorioLogs[MAX_RUTA] = "logs";
static char rutaArchivoLog[MAX_RUTA] = "logs/minisync.log";

static pid_t pidsWorkers[NUM_WORKERS]; //PID de los workers
//Declaracion de funciones:
long copiarArchivo(char *origen,char *destino);
static void ejecutarWorker(int numeroWorker);
static void construirRutaDestino(char *rutaOrigen,char *rutaDestino);
static void copiarInformacionArchivo(Archivo archivoOrigen,Archivo *archivoDestino);
static void liberarArregloArchivos(Archivo arreglo[],int cantidad);
static int buscarArchivoAnterior(Archivo archivosAnteriores[],int cantidadAnterior,char *ruta);
static volatile sig_atomic_t continuar = 1;// Controla el ciclo del Monitor


// MANEJA CTRL+C para convertir el monitor en un Daemon 
static void manejarSenial(int senial){
    (void)senial;

    continuar = 0;
}

// INICIA EL MONITOR DEL DIRECTORIO:
void iniciarMonitor(char *directorio){

    // Guarda el escaneo anterior
    Archivo archivosAnteriores[MAX_ARCHIVOS];
    int cantidadAnterior = 0;
    // Guarda los archivos modificados
    Archivo archivosModificados[MAX_ARCHIVOS];
    int cantidadModificados = 0;
    
    signal(SIGINT,manejarSenial);
    signal(SIGTERM,manejarSenial);

    
    inicializarIPC();// Crea los pipes, memoria compartida y semáforo
    iniciarLogger();// Crea el proceso logger
    crearWorkers();// Crea los dos procesos workers

    // PRIMER ESCANEO
    scan(directorio);
    printf("\n---------- ESCANEO INICIAL ----------\n");

    for(int i = 0; i < cantidad_archivos; i++){
        imprimirArchivo(archivos[i]);
        copiarInformacionArchivo(archivos[i],&archivosAnteriores[i]);
    }

    cantidadAnterior = cantidad_archivos;
    printf("\nEsperando modificaciones...\n");

    // MONITOREO PERIÓDICO
    while(continuar){
        sleep(5);
        if(continuar == 0){
            break;
        }
        mostrarEstadisticas();// Muestra lo realizado durante los 5 segundos anteriores
        // Vuelve a recorrer el directorio
        scan(directorio);
        cantidadModificados = 0;

        printf("\n---------- NUEVO ESCANEO ----------\n");
        for(int i = 0; i < cantidad_archivos; i++){

            int posicionAnterior;

            posicionAnterior =buscarArchivoAnterior(archivosAnteriores,cantidadAnterior,archivos[i].ruta);

            // Si no existía antes, es un archivo nuevo
            if(posicionAnterior == -1){
                printf("    [MONITOR] Archivo nuevo: %s\n",archivos[i].ruta);
                copiarInformacionArchivo(archivos[i],&archivosModificados[cantidadModificados]);
                cantidadModificados++;
            }else{
                int cambio;
                cambio = compararArchivo(archivosAnteriores[posicionAnterior],archivos[i]);

                if(cambio == 1){
                    printf("    [MONITOR] Archivo modificado: %s\n",archivos[i].ruta);
                    printf("    Tamaño anterior: %lld bytes\n",(long long)archivosAnteriores[posicionAnterior].tamanio);
                    printf("    Tamaño actual: %lld bytes\n",(long long)archivos[i].tamanio);
                    copiarInformacionArchivo(archivos[i],&archivosModificados[cantidadModificados]);
                    cantidadModificados++;
                }
            }
        }

        printf("-- Archivos nuevos o modificados: %d --\n",cantidadModificados);

        // REPARTIR LOS ARCHIVOS ENTRE LOS WORKERS
        for(int i = 0; i < cantidadModificados; i++){
            int numeroWorker;
            // Reparto alternado:
            // archivo 0 → Worker 1
            // archivo 1 → Worker 2
            // archivo 2 → Worker 1
            // archivo 3 → Worker 2
            numeroWorker = i % NUM_WORKERS;
            int resultado;
            resultado = enviarTarea(numeroWorker,archivosModificados[i].ruta);

            if(resultado == 1){
                printf("[Monitor] %s asignado al Worker %d\n",archivosModificados[i].ruta,numeroWorker + 1);
            }else{
                printf("[Monitor] No se pudo enviar %s al Worker %d\n",archivosModificados[i].ruta,numeroWorker + 1);
            }
        }

        // Libera el escaneo anterior
        liberarArregloArchivos(archivosAnteriores,cantidadAnterior);

        // El escaneo actual se convierte
        // en el nuevo escaneo anterior
        for(int i = 0; i < cantidad_archivos; i++){
            copiarInformacionArchivo(archivos[i],&archivosAnteriores[i]);
        }

        cantidadAnterior = cantidad_archivos;

        // Todavía no mandamos estos archivos a los workers, por eso los liberamos
        liberarArregloArchivos(archivosModificados,cantidadModificados);
    }

    printf("\n[Monitor] Finalizando MiniFileSync...\n");
    // Envía FIN a cada worker
    for(int i = 0; i < NUM_WORKERS; i++){
        enviarTarea(i,"FIN");
    }
    // Espera a los dos workers
    for(int i = 0; i < NUM_WORKERS; i++){
        waitpid(pidsWorkers[i],NULL,0);
    }
    // Libera el último escaneo guardado
    liberarArregloArchivos(archivosAnteriores,cantidadAnterior);
    // Finaliza el Logger
    detenerLogger();
    // Libera pipes, memoria y semáforo
    liberarIPC();
    printf("[Monitor] MiniFileSync finalizado.\n");
}


int compararArchivo(Archivo archivoAnterior,Archivo archivoActual){
    // Si cambió el tamaño, el archivo fue modificado
    if(archivoAnterior.tamanio!= archivoActual.tamanio){
        return 1;
    }
    // Si cambió la fecha, el archivo fue modificado
    if(archivoAnterior.fecha_modificacion!= archivoActual.fecha_modificacion){
        return 1;
    }

    // No se detectaron cambios
    return 0;
}

// COPIA LOS DATOS DE UN ARCHIVO:
void copiarInformacionArchivo(Archivo archivoOrigen,Archivo *archivoDestino){
    // Reserva memoria para copiar el nombre
    archivoDestino->nombre = malloc(strlen(archivoOrigen.nombre) + 1);

    // Verifica que malloc haya funcionado
    if(archivoDestino->nombre == NULL){
        printf("Error al reservar memoria para el nombre.\n");
        return;
    }

    strcpy(archivoDestino->nombre,archivoOrigen.nombre);

    // Reserva memoria para copiar la ruta
    archivoDestino->ruta = malloc(strlen(archivoOrigen.ruta) + 1);

    if(archivoDestino->ruta == NULL){
        printf("Error al reservar memoria para la ruta.\n");
        free(archivoDestino->nombre);
        archivoDestino->nombre = NULL;
        return;
    }

    strcpy(archivoDestino->ruta,archivoOrigen.ruta);

    // Copia los demás metadatos
    archivoDestino->inodo = archivoOrigen.inodo;
    archivoDestino->tamanio = archivoOrigen.tamanio;
    archivoDestino->permisos = archivoOrigen.permisos;
    archivoDestino->fecha_modificacion = archivoOrigen.fecha_modificacion;
}

// LIBERA UN ARREGLO DE ARCHIVOS
void liberarArregloArchivos(Archivo arreglo[],int cantidad){
    for(int i = 0; i < cantidad; i++){
        free(arreglo[i].nombre);
        free(arreglo[i].ruta);
        arreglo[i].nombre = NULL;
        arreglo[i].ruta = NULL;
    }
}

// BUSCA UN ARCHIVO POR SU RUTA
int buscarArchivoAnterior(Archivo archivosAnteriores[],int cantidadAnterior,char *ruta){
    for(int i = 0; i < cantidadAnterior; i++){
        if(strcmp(archivosAnteriores[i].ruta,ruta) == 0){
            return i;
        }
    }

    // No existía durante el escaneo anterior
    return -1;
}

void crearWorkers(){

    for(int i = 0; i < NUM_WORKERS; i++){
        pid_t pid;
        pid = fork();

        if(pid < 0){
            perror("Error al crear worker");
            exit(EXIT_FAILURE);
        }

        if(pid == 0){ // Estamos dentro del hijo, el worker no escribe en ningún pipe, solo conserva la lectura de su pipe
            for(int j = 0; j < NUM_WORKERS; j++){
                // Ningún worker escribe tareas
                close(tuberias[j][1]);
                // Cierra la lectura de los otros workers
                if(j != i){
                    close(tuberias[j][0]);
                }
            }
            ejecutarWorker(i);
        }

        // Solo el Monitor llega aquí
        pidsWorkers[i] = pid;

        printf("[Monitor] Worker %d creado. PID: %d\n",i + 1,pid);
    }


    for(int i = 0; i < NUM_WORKERS; i++){
        close(tuberias[i][0]);
        tuberias[i][0] = -1;
    }
}

static void construirRutaDestino(char *rutaOrigen,char *rutaDestino){
    char *nombreArchivo;

    nombreArchivo = strrchr(rutaOrigen, '/');


    if(nombreArchivo == NULL){
        nombreArchivo = rutaOrigen;
    }else{
        nombreArchivo++;
    }

    snprintf(rutaDestino,MAX_RUTA,"backup/%s",nombreArchivo);
}

static void ejecutarWorker(int numeroWorker){
    char rutaOrigen[MAX_RUTA];
    char rutaDestino[MAX_RUTA];

    printf("    [Worker %d] Iniciado. PID: %d\n",numeroWorker + 1,getpid());

    while(1){
        int resultado;
        resultado = recibirTarea(numeroWorker,rutaOrigen);

        if(resultado <= 0){
            break;
        }
        // Mensaje especial para finalizar
        if(strcmp(rutaOrigen, "FIN") == 0){
            break;
        }
        construirRutaDestino(rutaOrigen,rutaDestino);
        printf("    [Worker %d] Copiando: %s\n",numeroWorker + 1,rutaOrigen);

        long bytesCopiados;
        bytesCopiados = copiarArchivo(rutaOrigen,rutaDestino);

        if(bytesCopiados >= 0){
            registrarCopia(bytesCopiados);
            printf("    [Worker %d] Copia terminada: %s\n",numeroWorker + 1,rutaDestino);

            char mensajeLog[MAX_RUTA + 100];
            snprintf(mensajeLog,sizeof(mensajeLog),"Worker %d copió %s",numeroWorker + 1,rutaDestino);
            enviarLog(mensajeLog);

        }else{
            registrarError();
            printf("    [Worker %d] Error al copiar: %s\n",numeroWorker + 1,rutaOrigen);

            char mensajeLog[MAX_RUTA + 100];
            snprintf(mensajeLog,sizeof(mensajeLog),"Worker %d presentó un error al copiar %s",numeroWorker + 1,rutaOrigen);
            enviarLog(mensajeLog);
        }
    }

    close(tuberias[numeroWorker][0]);

    printf("[Worker %d] Finalizado.\n",numeroWorker + 1);

    exit(EXIT_SUCCESS);
}

void mostrarEstadisticas(){

    Stats datos;

    datos = obtenerEstadisticas();

    mostrarStats(datos);
}

// INICIA EL MONITOR COMO DAEMON
void iniciarDaemon(char *directorio){

    char directorioAbsoluto[MAX_RUTA];
    char directorioProyecto[MAX_RUTA];


    //Convierte el directorio monitorizado en una ruta absoluta antes de chdir("/").
    if(realpath(directorio,directorioAbsoluto) == NULL){
        perror("Error al obtener la ruta absoluta");
        return;
    }


    // Guarda la carpeta actual del proyecto, ahí estarán backup/ y logs/.
    if(getcwd(directorioProyecto,sizeof(directorioProyecto)) == NULL){
        perror("Error al obtener el directorio actual");
        return;
    }

    snprintf(directorioBackup,sizeof(directorioBackup),"%s/backup",directorioProyecto);
    snprintf(directorioLogs,sizeof(directorioLogs),"%s/logs",directorioProyecto);
    snprintf(rutaArchivoLog,sizeof(rutaArchivoLog),"%s/logs/minisync.log",directorioProyecto);

    // Crea las carpetas antes de separarse
    mkdir(directorioBackup,0755);
    mkdir(directorioLogs,0755);

    pid_t pid;
    pid = fork();

    if(pid < 0){
        perror("Error al crear el daemon");
        return;
    }

    // El proceso padre termina
    if(pid > 0){
        printf("MiniFileSync ejecutándose como daemon.\n");
        printf("PID del daemon: %d\n",pid);
        printf("Para detenerlo: ./main stop\n");
        return;
    }

    //El hijo crea una nueva sesión y se separa de la terminal.
    if(setsid() == -1){
        exit(EXIT_FAILURE);
    }

    // Cambia el directorio de trabajo a /
    if(chdir("/") == -1){
        exit(EXIT_FAILURE);
    }

    // Permisos definidos por cada open()
    umask(0);

    // Redirige entrada y salida a /dev/null
    int fdNulo;
    fdNulo = open("/dev/null",O_RDWR);

    if(fdNulo != -1){
        dup2(fdNulo,STDIN_FILENO);
        dup2(fdNulo,STDOUT_FILENO);
        dup2(fdNulo,STDERR_FILENO);
        if(fdNulo > STDERR_FILENO){
            close(fdNulo);
        }
    }

    // Guarda el PID del daemon
    int fdPID;
    fdPID = open(ARCHIVO_PID,O_WRONLY | O_CREAT | O_TRUNC,0644);

    if(fdPID != -1){
        char textoPID[50];
        int longitud;
        longitud = snprintf(textoPID,sizeof(textoPID),"%d\n",getpid());

        write(fdPID,textoPID,longitud);
        close(fdPID);
    }


    // Ejecuta el Monitor usando la ruta absoluta
    iniciarMonitor(directorioAbsoluto);

    exit(EXIT_SUCCESS);
}

// DETIENE EL DAEMON
int detenerDaemon(){

    int fdPID;
    fdPID = open(ARCHIVO_PID,O_RDONLY);

    if(fdPID == -1){
        printf("No existe un daemon activo.\n");
        return -1;
    }

    char textoPID[50];
    memset(textoPID,0,sizeof(textoPID));

    ssize_t bytesLeidos;
    bytesLeidos = read(fdPID,textoPID,sizeof(textoPID) - 1);
    close(fdPID);

    if(bytesLeidos <= 0){
        printf("No se pudo leer el PID del daemon.\n");
        return -1;
    }
    pid_t pid;
    pid = (pid_t)atoi(textoPID);

    if(kill(pid, SIGTERM) == -1){
        perror("Error al detener el daemon");
        return -1;
    }
    printf("Se envió la señal de cierre al daemon PID %d.\n",pid);


    return 0;
}