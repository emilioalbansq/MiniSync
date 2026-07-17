#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include <semaphore.h>

#include "../include/ipc.h"



#define NOMBRE_MEMORIA "/minifilesync_stats"// Nombre de la memoria compartida
#define NOMBRE_SEMAFORO "/minifilesync_sem" // Nombre del semáforo
Stats *estadisticas = NULL;// Memoria compartida de estadísticas
int tuberias[NUM_WORKERS][2];// Dos pipes: uno por cada worker
static int fdMemoria = -1; //descriptor default de la memoria compartida
// Semáforo usado para proteger las estadísticas
static sem_t *semaforoEstadisticas = SEM_FAILED;

void inicializarIPC(){

    printf("\n");
    printf("{          INICIALIZANDO IPC        }\n");

    // CREAR UN PIPE PARA CADA WORKER
    for(int i = 0; i < NUM_WORKERS; i++){
        if(pipe(tuberias[i]) == -1){
            printf("Error al crear el pipe del Worker %d.\n",i + 1);
            exit(1);
        }
        printf("Pipe del Worker %d creado correctamente.\n",i + 1);
    }

    // CREAR MEMORIA COMPARTIDA
    shm_unlink(NOMBRE_MEMORIA);
    fdMemoria = shm_open(NOMBRE_MEMORIA,O_CREAT | O_RDWR,0666);

    if(fdMemoria == -1){
        printf("Error al crear la memoria compartida.\n");
        exit(1);
    }

    // Asigna el tamaño de la estructura Stats
    if(ftruncate(fdMemoria,sizeof(Stats)) == -1){
        printf("Error al establecer el tamaño de la memoria compartida.\n");
        exit(1);
    }

    // Conecta la memoria compartida
    estadisticas = mmap(NULL,sizeof(Stats),PROT_READ | PROT_WRITE,MAP_SHARED,fdMemoria,0);

    if(estadisticas == MAP_FAILED){
        printf("Error al mapear la memoria compartida.\n");
        exit(1);
    }
    // CREAR SEMÁFORO:
    sem_unlink(NOMBRE_SEMAFORO);
    semaforoEstadisticas = sem_open(NOMBRE_SEMAFORO,O_CREAT,0666,1);
    if(semaforoEstadisticas == SEM_FAILED){
        printf("Error al crear el semáforo.\n");
        exit(1);
    }

    // INICIALIZAR ESTADÍSTICAS
    inicializarStats(estadisticas);
    printf("Memoria compartida y semáforo creados.\n");
}

int enviarTarea(int numeroWorker,char *archivo){
    char mensaje[MAX_RUTA];

    // Valida el número del worker
    if(numeroWorker < 0|| numeroWorker >= NUM_WORKERS){
        printf("Número de worker incorrecto.\n");
        return -1;
    }

    // Limpia todo el mensaje
    memset(mensaje,0,sizeof(mensaje));

    // Verifica que la ruta tenga espacio
    if(strlen(archivo) >= MAX_RUTA){
        printf("La ruta es demasiado larga.\n");
        return -1;
    }
    strcpy(mensaje,archivo);

    ssize_t bytesEscritos;

    bytesEscritos = write(tuberias[numeroWorker][1],mensaje,sizeof(mensaje));

    if(bytesEscritos == -1){
        printf("Error al enviar tarea al Worker %d.\n",numeroWorker + 1);
        return -1;
    }

    if(bytesEscritos != sizeof(mensaje)){
        printf("La tarea no se envió completamente.\n");
        return -1;
    }
    
    return 1;
}

int recibirTarea(int numeroWorker,char *archivo){
    // Limpia el arreglo antes de leer
    memset(archivo,0,MAX_RUTA);
    ssize_t bytesLeidos;
    bytesLeidos = read(tuberias[numeroWorker][0],archivo,MAX_RUTA);
    if(bytesLeidos == -1){
        printf("Error al recibir tarea en Worker %d.\n",numeroWorker + 1);
        return -1;
    }
    if(bytesLeidos == 0){
        return 0;// El Monitor cerró el extremo de escritura
    }

    return 1;
}

void registrarCopia(long bytes){
    // Bloquea las estadísticas
    sem_wait(semaforoEstadisticas);
    estadisticas->archivos_copiados = estadisticas->archivos_copiados + 1;
    estadisticas->bytes_copiados = estadisticas->bytes_copiados + bytes;
    // Desbloquea las estadísticas
    sem_post(semaforoEstadisticas);
}

void registrarError(){
    sem_wait(semaforoEstadisticas);
    estadisticas->errores = estadisticas->errores + 1;
    sem_post(semaforoEstadisticas);
}

Stats obtenerEstadisticas(){
    Stats copia;
    sem_wait(semaforoEstadisticas);
    copia.archivos_copiados =estadisticas->archivos_copiados;
    copia.bytes_copiados =estadisticas->bytes_copiados;
    copia.errores =estadisticas->errores;
    sem_post(semaforoEstadisticas);
    return copia;
}

void liberarIPC(){

    // Cierra los pipes
    for(int i = 0; i < NUM_WORKERS; i++){
        if(tuberias[i][0] != -1){ // Si el pipe de lectura está abierto
            close(tuberias[i][0]);
            tuberias[i][0] = -1;
        }
        if(tuberias[i][1] != -1){ // Si el pipe de escritura está abierto
            close(tuberias[i][1]);
            tuberias[i][1] = -1;
        }
    }

    // Libera el mapeo
    if(estadisticas != NULL&& estadisticas != MAP_FAILED){
        munmap(estadisticas,sizeof(Stats));
        estadisticas = NULL;
    }

    // Cierra el descriptor de memoria
    if(fdMemoria  != -1){
        close(fdMemoria );
        fdMemoria = -1;
    }
    // Elimina la memoria compartida
    shm_unlink(NOMBRE_MEMORIA);

    // Cierra y elimina el semáforo
    if(semaforoEstadisticas != SEM_FAILED){
        sem_close(semaforoEstadisticas);
        semaforoEstadisticas = SEM_FAILED;
    }
    sem_unlink(NOMBRE_SEMAFORO);
    printf("[IPC] Recursos liberados.\n");
}