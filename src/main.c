#include <stdio.h>
#include <string.h>

#include "../include/scanner.h"
#include "../include/monitor.h"

// Posteriormente estará implementada
void iniciarDaemon(char *directorio);

void mostrarAyuda(char *nombrePrograma){
    printf("\n");
    printf("MiniFileSync - Emilio Albán\n");
    printf("Uso:\n");

    printf("  %s scan <directorio>\n",nombrePrograma);
    printf("  %s monitor <directorio>\n",nombrePrograma);
    printf("  %s daemon <directorio>\n",nombrePrograma);
    printf("  %s help\n",nombrePrograma);
    printf("\n");
    printf("scan: realiza un escaneo único.\n");
    printf("monitor: sincroniza cada 5 segundos ""en primer plano.\n");
    printf("daemon: sincroniza en segundo plano.\n");
}

int main(int argc, char *argv[]){

    // No se escribió ningún comando
    if(argc < 2){
        mostrarAyuda(argv[0]);
        return 1;
    }

    // AYUDA
    if(strcmp(argv[1], "help") == 0){
        mostrarAyuda(argv[0]);
        return 0;
    }

    //Detener proceso demonio
    if(strcmp(argv[1],"stop") == 0){
        return detenerDaemon();
    }

    // Los demás comandos requieren directorio
    if(argc != 3){
        printf("Error: debe indicar un directorio.\n");
        mostrarAyuda(argv[0]);
        return 1;
    }

    // ESCANEO ÚNICO
    if(strcmp(argv[1], "scan") == 0){
        scan(argv[2]);
        return 0;
    }

    // MONITOR EN PRIMER PLANO
    if(strcmp(argv[1], "monitor") == 0){
        iniciarMonitor(argv[2]);
        return 0;
    }

    // MONITOR COMO DAEMON
    if(strcmp(argv[1], "daemon") == 0){
        iniciarDaemon(argv[2]);
        return 0;
    }

    printf("Error: comando desconocido '%s'.\n",argv[1]);
    mostrarAyuda(argv[0]);

    return 1;
}