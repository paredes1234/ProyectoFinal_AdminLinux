include <stdio.h>
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
