#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "../include/scanner.h"

// ARREGLO DONDE SE GUARDAN LOS ARCHIVOS ENCONTRADOS DURANTE EL ESCANEO
Archivo archivos[MAX_ARCHIVOS];
int cantidad_archivos = 0; // Cantidad actual de archivos encontrados

// INICIA EL ESCANEO DEL DIRECTORIO
void scan(char *directorio){

    // Libera la memoria del escaneo anterior
    for(int i = 0; i < cantidad_archivos; i++){
        free(archivos[i].nombre);
        free(archivos[i].ruta);
        archivos[i].nombre = NULL;
        archivos[i].ruta = NULL;
    }
    // Reinicia la cantidad de archivos encontrados
    cantidad_archivos = 0;

    printf("\n");
    printf("-------------------------------------\n");
    printf("        ESCANEO DE DIRECTORIO        \n");
    printf("-------------------------------------\n");

    // Comienza el recorrido recursivo
    escanearDirectorio(directorio);

    printf("\n");
    printf("    Cantidad de archivos encontrados: %d\n", cantidad_archivos);

}

void escanearDirectorio(char *ruta){

    DIR *directorio; // Variable que representa el directorio abierto
    struct dirent *entrada; //Apunta al archivo que readdir() leerá

    directorio = opendir(ruta); // Abre el directorio recibido

    if(directorio == NULL){// Verifica si pudo abrir el directorio
        printf("No se pudo abrir el directorio: %s\n", ruta);
        return;
    }

    while((entrada = readdir(directorio)) != NULL){
        // Ignora "."(no quedar en el bucle del mismo directorio) y ".."(no recorrer el directorio padre)
        if(strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0){
            continue; //ignora esta vuelta del bucle y pasa al siguiente archivo
        }
        // Guardamos el nombre del archivo en una variable propia
        char nombre[256];
        strcpy(nombre, entrada->d_name);
        // Construye la ruta completa del archivo
        char rutaCompleta[1024];
        snprintf(rutaCompleta,sizeof(rutaCompleta),"%s/%s",ruta,nombre);

        // Obtiene la información del archivo usando stat()
        struct stat informacion;
        stat(rutaCompleta,&informacion); //copia todos esos datos dentro de nuestra variable informacion

        // Si encuentra otra carpeta, entra recursivamente
        if(S_ISDIR(informacion.st_mode)){
            escanearDirectorio(rutaCompleta);
        } else {
            Archivo archivo;
            archivo.nombre = malloc(strlen(nombre)+1); //reservamos espacio para el nombre del archivo
            strcpy(archivo.nombre,nombre); //copiamos el nombre del archivo en la estructura

            archivo.ruta = malloc(strlen(rutaCompleta)+1); //reservamos espacio para la ruta completa del archivo
            strcpy(archivo.ruta,rutaCompleta); //copiamos la ruta completa del archivo en la estructura
            archivo.inodo = informacion.st_ino; // almacenamos el número de i-nodo del archivo
            archivo.tamanio = informacion.st_size; // almacenamos el tamaño del archivo en bytes
            archivo.permisos = informacion.st_mode; // almacenamos los permisos del archivo
            archivo.fecha_modificacion = informacion.st_mtime; // almacenamos la fecha de última modificación del archivo
            
            archivos[cantidad_archivos] = archivo; //almacenamos la estructura del archivo en el arreglo de archivos encontrados
            cantidad_archivos++; //incrementa la cantidad de archivos encontrados
        }
    }

    closedir(directorio);

}

void imprimirArchivo(Archivo archivo){

    struct tm *fecha; //Variable para almacenar la fecha de modificación del archivo
    char fechaTexto[100]; //Variable para almacenar la fecha de modificación del archivo en formato de texto

    fecha = localtime(&archivo.fecha_modificacion); //convierte la fecha de modificación del archivo a una estructura tm

    strftime(fechaTexto,sizeof(fechaTexto),"%d/%m/%Y %H:%M:%S",fecha);//convierte la estructura tm a una cadena de texto con el formato deseado

    printf("\n========================================\n");
    printf("    Nombre: %s\n", archivo.nombre);
    printf("    Ruta: %s\n", archivo.ruta);
    printf("    Inodo: %lu\n", (unsigned long)archivo.inodo); // entero sin signo largo
    printf("    Tamaño: %lld bytes\n", (long long)archivo.tamanio); // entero largo largo
    printf("    Permisos: ");
    // Imprime los permisos del archivo en formato rwxrwxrwx
    printf("%c",(archivo.permisos & S_IRUSR) ? 'r' : '-'); 
    printf("%c",(archivo.permisos & S_IWUSR) ? 'w' : '-');
    printf("%c",(archivo.permisos & S_IXUSR) ? 'x' : '-');

    printf("%c",(archivo.permisos & S_IRGRP) ? 'r' : '-');
    printf("%c",(archivo.permisos & S_IWGRP) ? 'w' : '-');
    printf("%c",(archivo.permisos & S_IXGRP) ? 'x' : '-');

    printf("%c",(archivo.permisos & S_IROTH) ? 'r' : '-');
    printf("%c",(archivo.permisos & S_IWOTH) ? 'w' : '-');
    printf("%c",(archivo.permisos & S_IXOTH) ? 'x' : '-');

    printf("\n");

    printf("Fecha modificación: %s\n", fechaTexto);

    printf("========================================\n");
}