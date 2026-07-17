#ifndef MONITOR_H
#define MONITOR_H

#include "file.h"

void iniciarMonitor(); // Inicia el proceso monitor
void iniciarDaemon();
int detenerDaemon();
int compararArchivo(Archivo archivoAnterior, Archivo archivoActual);// Compara dos archivos mediante sus metadatos
void crearWorkers();// Crea los procesos workers (fork(), pipes y asignación de archivos)
void mostrarEstadisticas();// Muestra las estadísticas de la sincronización en consola

#endif