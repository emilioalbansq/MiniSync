#ifndef IPC_H
#define IPC_H

#include "stats.h"

#define NUM_WORKERS 2
#define MAX_RUTA 1024

// Memoria compartida para estadísticas
extern Stats *estadisticas;

/*
 * Pipe por Worker:
 * tuberias[0] es el pipe del Worker 1
 * tuberias[1] es el  pipe del Worker 2
 * tuberias[i][0] para lectura
 * tuberias[i][1] para escritura
 */
extern int tuberias[NUM_WORKERS][2];


void inicializarIPC();// Inicializa pipes, memoria compartida y semáforo
int enviarTarea(int numeroWorker, char *archivo);// Monitor envía una tarea al worker indicado
int recibirTarea(int numeroWorker, char *archivo);// Worker recibe una tarea del Monitor
void registrarCopia(long bytes);// Actualiza las estadísticas de forma segura
void registrarError();
Stats obtenerEstadisticas();// Devuelve una copia segura de las estadísticas
void liberarIPC();// Libera todos los recursos IPC
#endif