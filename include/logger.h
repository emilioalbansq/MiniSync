#ifndef LOGGER_H
#define LOGGER_H

void iniciarLogger();// Crea el FIFO y el proceso Logger
void enviarLog(char *mensaje);// Envía un mensaje desde un worker al Logger
void detenerLogger();// Finaliza y espera al proceso Logger

#endif