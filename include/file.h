#ifndef FILE_H
#define FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
typedef struct archivo{
    char *nombre;               // Nombre del archivo (ej: foto.jpg)
    char *ruta;                 // Ruta completa (ej: originales/documentos/foto.jpg)
    ino_t inodo;                // Número de i-nodo
    off_t tamanio;               // Tamaño del archivo en bytes
    mode_t permisos;            // Permisos del archivo
    time_t fecha_modificacion;  // Fecha de última modificación
} Archivo;
#endif
