#ifndef SCANNER_H
#define SCANNER_H

#include "file.h"
#define MAX_ARCHIVOS 100

//ARREGLO DONDE SE ALMACENAN LOS ARCHIVOS ENCONTRADOS DURANTE EL ESCANEO
extern Archivo archivos[MAX_ARCHIVOS];
//Cantidad actual de archivos almacenados
extern int cantidad_archivos;


void scan(char *directorio);//Inicia el escaneo desde un directorio
void escanearDirectorio(char *ruta);//Recorre recursivamente los directorios
Archivo obtenerArchivo(char *ruta);// Obtiene los metadatos de un archivo
void imprimirArchivo(Archivo archivo);// Imprime los metadatos
void liberarArchivo(Archivo archivo);// Libera memoria reservada
#endif