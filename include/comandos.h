#ifndef COMANDOS_H
#define COMANDOS_H

typedef struct {
    char *salida_estandar; /* el llamador debe hacer free() */
    char *salida_error;    /* el llamador debe hacer free() */
    int codigo_salida;
} ResultadoComando;