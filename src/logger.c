#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define RUTA_FIFO "/tmp/minifilesync_logger"
#define MAX_LOG 1024

static pid_t pidLogger = -1;


// INICIA EL PROCESO LOGGER
void iniciarLogger(){

    // Crea el FIFO si todavía no existe
    if(mkfifo(RUTA_FIFO, 0666) == -1 && errno != EEXIST){
        perror("Error al crear FIFO del logger");
        exit(EXIT_FAILURE);
    }

    pidLogger = fork();
    if(pidLogger < 0){
        perror("Error al crear el logger");
        exit(EXIT_FAILURE);
    }

    if(pidLogger == 0){
        // Proceso hijo: Logger
        int fdFIFO;
        fdFIFO = open(RUTA_FIFO, O_RDWR);

        if(fdFIFO == -1){
            perror("Error al abrir FIFO del logger");
            exit(EXIT_FAILURE);
        }

        int fdLog;
        fdLog = open("logs/minisync.log",O_WRONLY | O_CREAT | O_APPEND,0644);

        if(fdLog == -1){
            perror("Error al abrir archivo de log");
            close(fdFIFO);
            exit(EXIT_FAILURE);
        }

        printf("[Logger] Iniciado. PID: %d\n",getpid());

        while(1){
            char mensaje[MAX_LOG];
            memset(mensaje,0,sizeof(mensaje));

            ssize_t bytesLeidos;
            bytesLeidos = read(fdFIFO,mensaje,sizeof(mensaje));

            if(bytesLeidos == -1){
                if(errno == EINTR){
                    continue;
                }
                perror("Error al leer FIFO");
                break;
            }

            if(strcmp(mensaje, "FIN_LOGGER") == 0){
                break;
            }

            // Obtener fecha y hora
            time_t tiempoActual;
            tiempoActual = time(NULL);
            struct tm *fecha;
            fecha = localtime(&tiempoActual);
            char fechaTexto[100];
            strftime(fechaTexto,sizeof(fechaTexto),"%Y-%m-%d %H:%M:%S",fecha);

            char linea[MAX_LOG + 150];
            int longitud;
            longitud = snprintf(linea,sizeof(linea),"[%s] %s\n",fechaTexto,mensaje);

            write(fdLog,linea,longitud);
            fsync(fdLog);
        }
        close(fdFIFO);
        close(fdLog);
        printf("[Logger] Finalizado.\n");
        exit(EXIT_SUCCESS);
    }

    printf("[Monitor] Logger creado. PID: %d\n",pidLogger);
}


// ENVÍA UN MENSAJE AL LOGGER
void enviarLog(char *mensaje){

    int fdFIFO;
    fdFIFO = open(RUTA_FIFO,O_WRONLY);

    if(fdFIFO == -1){
        printf("Error al comunicarse con el logger.\n");
        return;
    }

    char mensajeCompleto[MAX_LOG];
    memset(mensajeCompleto,0,sizeof(mensajeCompleto));
    snprintf(mensajeCompleto,sizeof(mensajeCompleto),"%s",mensaje);
    write(fdFIFO,mensajeCompleto,sizeof(mensajeCompleto));
    close(fdFIFO);
}


// FINALIZA EL LOGGER
void detenerLogger(){
    if(pidLogger <= 0){
        return;
    }

    enviarLog("FIN_LOGGER");
    waitpid(pidLogger,NULL,0);
    unlink(RUTA_FIFO);

    pidLogger = -1;
}