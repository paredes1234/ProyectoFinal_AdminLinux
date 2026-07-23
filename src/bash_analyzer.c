#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/bash_analyzer.h"
#include "../include/utils.h"

#define MAX_VARS 256

static int es_asignacion_variable(const char *linea, char *nombre_var, int tam) {
    /* Busca patrón NOMBRE=valor al inicio de línea (sin espacios antes del =) */
    int i = 0;
    while (isspace((unsigned char) linea[i])) i++;

    int inicio = i;
    if (!isalpha((unsigned char) linea[i]) && linea[i] != '_') return 0;

    while (isalnum((unsigned char) linea[i]) || linea[i] == '_') i++;
    int fin = i;
    if (fin == inicio) return 0;
    if (linea[i] != '=') return 0;
    /* Evitar confundir con comparaciones == o !=, [ x = y ] */
    if (linea[i + 1] == '=') return 0;

    int len = fin - inicio;
    if (len >= tam) len = tam - 1;
    strncpy(nombre_var, linea + inicio, len);
    nombre_var[len] = '\0';
    return 1;
}

static int ya_registrada(char vars[][128], int total, const char *nombre) {
    for (int i = 0; i < total; i++) {
        if (strcmp(vars[i], nombre) == 0) return 1;
    }
    return 0;
}
