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
int  archivos_copiar(const char *origen, const char *destino);
int  archivos_mover(const char *origen, const char *destino);
int  archivos_eliminar(const char *ruta);
int  archivos_crear_directorio(const char *ruta);
int  archivos_crear_archivo(const char *ruta);
void archivos_buscar(const char *ruta_base, const char *patron);
void archivos_estadisticas(const char *ruta);

/* Variantes que devuelven datos estructurados (usadas por la GUI) */
ListaArchivos archivos_obtener_lista(const char *ruta);
ListaArchivos archivos_buscar_lista(const char *ruta_base, const char *patron);
void archivos_lista_liberar(ListaArchivos *lista);
char *archivos_estadisticas_texto(const char *ruta); /* el llamador debe hacer free() */

#endif
