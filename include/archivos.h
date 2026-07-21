#ifndef ARCHIVOS_H
#define ARCHIVOS_H

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    char nombre[256];
    char ruta[PATH_MAX];
    int es_directorio;
    long long tamano;
} ItemArchivo;

typedef struct {
    ItemArchivo *items;
    int total;
    int truncada; /* 1 cuando la búsqueda alcanzó el límite de resultados */
} ListaArchivos;

void archivos_listar(const char *ruta);



#endif