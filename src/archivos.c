#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include "../include/archivos.h"
#include "../include/utils.h"

#define LIMITE_RESULTADOS_BUSQUEDA 5000

static int construir_ruta(char *salida, size_t tam, const char *base, const char *nombre) {
    int escritos;
    if (strcmp(base, "/") == 0)
        escritos = snprintf(salida, tam, "/%s", nombre);
    else
        escritos = snprintf(salida, tam, "%s/%s", base, nombre);
    return escritos >= 0 && (size_t) escritos < tam;
}

void archivos_listar(const char *ruta) {
    DIR *d = opendir(ruta);
    if (!d) {
        printf(COLOR_ERROR "No se pudo abrir: %s\n" COLOR_RESET, ruta);
        return;
    }

    utils_titulo("Contenido del directorio");
    printf("%-30s %-10s %-12s\n", "NOMBRE", "TIPO", "TAMAÑO");
    printf("------------------------------------------------\n");

    struct dirent *entrada;
    char ruta_completa[PATH_MAX];
    struct stat st;
    char tam_fmt[32];

    while ((entrada = readdir(d)) != NULL) {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0)
            continue;
        if (!construir_ruta(ruta_completa, sizeof(ruta_completa), ruta, entrada->d_name))
            continue;
        if (lstat(ruta_completa, &st) != 0) continue;

        const char *tipo = S_ISDIR(st.st_mode) ? "DIR" :
                           S_ISLNK(st.st_mode) ? "ENLACE" : "ARCHIVO";
        utils_formatear_bytes(st.st_size, tam_fmt, sizeof(tam_fmt));
        printf("%-30s %-10s %-12s\n", entrada->d_name, tipo, tam_fmt);
    }
    closedir(d);
}

static int comparar_items(const void *a, const void *b) {
    const ItemArchivo *ia = a;
    const ItemArchivo *ib = b;
    if (ia->es_directorio != ib->es_directorio)
        return ib->es_directorio - ia->es_directorio;
    return strcasecmp(ia->nombre, ib->nombre);
}