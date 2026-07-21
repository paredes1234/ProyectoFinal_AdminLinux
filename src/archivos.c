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

ListaArchivos archivos_obtener_lista(const char *ruta) {
    ListaArchivos lista = {NULL, 0, 0};
    DIR *d = opendir(ruta);
    if (!d) return lista;

    int capacidad = 64;
    lista.items = malloc(sizeof(ItemArchivo) * (size_t) capacidad);
    if (!lista.items) {
        closedir(d);
        return lista;
    }

    struct dirent *entrada;
    char ruta_completa[PATH_MAX];
    struct stat st;

    while ((entrada = readdir(d)) != NULL) {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0)
            continue;
        if (!construir_ruta(ruta_completa, sizeof(ruta_completa), ruta, entrada->d_name))
            continue;
        if (lstat(ruta_completa, &st) != 0) continue;

        if (lista.total >= capacidad) {
            capacidad *= 2;
            ItemArchivo *nuevos = realloc(lista.items,
                                           sizeof(ItemArchivo) * (size_t) capacidad);
            if (!nuevos) break;
            lista.items = nuevos;
        }

        ItemArchivo *item = &lista.items[lista.total++];
        snprintf(item->nombre, sizeof(item->nombre), "%s", entrada->d_name);
        snprintf(item->ruta, sizeof(item->ruta), "%s", ruta_completa);
        item->es_directorio = S_ISDIR(st.st_mode);
        item->tamano = (long long) st.st_size;
    }
    closedir(d);

    qsort(lista.items, (size_t) lista.total, sizeof(ItemArchivo), comparar_items);
    return lista;
}

void archivos_lista_liberar(ListaArchivos *lista) {
    if (!lista) return;
    free(lista->items);
    lista->items = NULL;
    lista->total = 0;
    lista->truncada = 0;
}

static int copiar_archivo_regular(const char *origen,
                                  const char *destino,
                                  mode_t permisos) {
    int fo = open(origen, O_RDONLY);
    if (fo < 0) return -1;

    int fd = open(destino, O_WRONLY | O_CREAT | O_TRUNC, permisos & 0777);
    if (fd < 0) {
        close(fo);
        return -1;
    }

    char buffer[64 * 1024];
    int resultado = 0;
    for (;;) {
        ssize_t leidos = read(fo, buffer, sizeof(buffer));
        if (leidos == 0) break;
        if (leidos < 0) {
            resultado = -1;
            break;
        }

        ssize_t enviados = 0;
        while (enviados < leidos) {
            ssize_t escritos = write(fd, buffer + enviados,
                                     (size_t) (leidos - enviados));
            if (escritos < 0) {
                resultado = -1;
                break;
            }
            enviados += escritos;
        }
        if (resultado != 0) break;
    }

    if (close(fo) != 0) resultado = -1;
    if (close(fd) != 0) resultado = -1;
    return resultado;
}

static int copiar_enlace_simbolico(const char *origen, const char *destino) {
    char objetivo[PATH_MAX];
    ssize_t longitud = readlink(origen, objetivo, sizeof(objetivo) - 1);
    if (longitud < 0) return -1;
    objetivo[longitud] = '\0';

    struct stat st_destino;
    if (lstat(destino, &st_destino) == 0) {
        errno = EEXIST;
        return -1;
    }
    return symlink(objetivo, destino);
}

static const char *nombre_base_ruta(const char *ruta) {
    const char *fin = ruta + strlen(ruta);
    while (fin > ruta + 1 && fin[-1] == '/') fin--;

    const char *inicio = fin;
    while (inicio > ruta && inicio[-1] != '/') inicio--;
    return inicio;
}

static int copiar_elemento_recursivo(const char *origen, const char *destino) {
    struct stat st;
    if (lstat(origen, &st) != 0) return -1;

    if (S_ISREG(st.st_mode))
        return copiar_archivo_regular(origen, destino, st.st_mode);

    if (S_ISLNK(st.st_mode))
        return copiar_enlace_simbolico(origen, destino);

    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTSUP;
        return -1;
    }

    if (mkdir(destino, st.st_mode & 0777) != 0) {
        /* Por seguridad no se mezclan carpetas existentes. */
        return -1;
    }

    DIR *directorio = opendir(origen);
    if (!directorio) {
        int error_guardado = errno;
        rmdir(destino);
        errno = error_guardado;
        return -1;
    }

    int resultado = 0;
    struct dirent *entrada;
    char ruta_origen[PATH_MAX];
    char ruta_destino[PATH_MAX];

    while ((entrada = readdir(directorio)) != NULL) {
        if (strcmp(entrada->d_name, ".") == 0 ||
            strcmp(entrada->d_name, "..") == 0)
            continue;

        if (!construir_ruta(ruta_origen, sizeof(ruta_origen),
                            origen, entrada->d_name) ||
            !construir_ruta(ruta_destino, sizeof(ruta_destino),
                            destino, entrada->d_name)) {
            errno = ENAMETOOLONG;
            resultado = -1;
            break;
        }

        if (copiar_elemento_recursivo(ruta_origen, ruta_destino) != 0) {
            resultado = -1;
            break;
        }
    }

    int error_guardado = errno;
    if (closedir(directorio) != 0 && resultado == 0) {
        resultado = -1;
        error_guardado = errno;
    }

    if (resultado == 0) {
        /* Restaurar permisos por si el umask los redujo al crear la carpeta. */
        chmod(destino, st.st_mode & 0777);
    }

    errno = error_guardado;
    return resultado;
}







