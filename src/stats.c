#include <stdio.h>

#include "../include/stats.h"

// INICIALIZA LAS ESTADÍSTICAS
void inicializarStats(Stats *datos){
    datos->archivos_copiados = 0;
    datos->bytes_copiados = 0;
    datos->errores = 0;
}
// MUESTRA LAS ESTADÍSTICAS
void mostrarStats(Stats datos){

    printf("\n");
    printf(".....................................\n");
    printf("    ESTADISTICAS DE SINCRONIZACION   \n");
    printf(".....................................\n");

    printf("Archivos copiados: %ld\n",datos.archivos_copiados);

    printf("Bytes copiados: %ld\n",datos.bytes_copiados);

    printf("Errores: %ld\n",datos.errores);
}