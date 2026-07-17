#ifndef STATS_H
#define STATS_H

typedef struct stats{
    long archivos_copiados;
    long bytes_copiados;
    long errores;
} Stats;


void inicializarStats();// Inicializa las estadísticas compartidas
void mostrarStats();// Muestra las estadísticas en consola

#endif