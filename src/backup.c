#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

long copiarArchivo(char *origen, char *destino){

    int fdOrigen; //Abrimos el archivo original.
    fdOrigen = open(origen, O_RDONLY);
    if(fdOrigen == -1){
        printf("Error al abrir el archivo origen.\n");
        return -1;
    }

    int fdDestino; //Abre/Crea el archivo destino
    fdDestino = open(destino,O_WRONLY | O_CREAT | O_TRUNC,0644);
    if(fdDestino == -1){
        printf("Error al crear el archivo destino.\n");
        close(fdOrigen);
        return -1;
    }

    char buffer[1024]; //Bloque de memoria para leer y escribir
    ssize_t bytesLeidos; //Lo que devuelve read() para saber cuántos bytes se leyeron
    long totalBytes = 0;

    while((bytesLeidos = read(fdOrigen,buffer,sizeof(buffer))) > 0){
        ssize_t bytesEscritos;
        bytesEscritos = write(fdDestino,buffer,bytesLeidos);
        totalBytes = totalBytes + bytesEscritos;
    }

    fsync(fdDestino); //asegura que los datos se escriban en el disco

    close(fdOrigen); //cierra el archivo origen
    close(fdDestino); //cierra el archivo destino

    return totalBytes;
}