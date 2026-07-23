#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "../include/respaldos.h"
#include "../include/utils.h"
#include "../include/archivos.h"

#define MAX_COLA 64

/* ---------- Respaldos incrementales ---------- */

/* Copia recursivamente origen -> destino, devolviendo cantidad de archivos copiados.
   Si min_mtime > 0, solo copia archivos modificados después de esa fecha (modo incremental). */
static int copiar_directorio_recursivo(const char *origen, const char *destino, time_t min_mtime) {
    mkdir(destino, 0755);

    DIR *d = opendir(origen);
    if (!d) return 0;

    int copiados = 0;
    struct dirent *entrada;
    char ruta_o[1024], ruta_d[1024];
    struct stat st;

    while ((entrada = readdir(d)) != NULL) {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) continue;
        snprintf(ruta_o, sizeof(ruta_o), "%s/%s", origen, entrada->d_name);
        snprintf(ruta_d, sizeof(ruta_d), "%s/%s", destino, entrada->d_name);

        if (stat(ruta_o, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            copiados += copiar_directorio_recursivo(ruta_o, ruta_d, min_mtime);
        } else {
            if (min_mtime == 0 || st.st_mtime > min_mtime) {
                if (archivos_copiar(ruta_o, ruta_d) == 0) copiados++;
            }
        }
    }
    closedir(d);
    return copiados;
}

/* Lee la fecha del último respaldo desde un manifiesto de control */
static time_t leer_ultimo_respaldo(const char *destino_base) {
    char ruta_manifiesto[1024];
    snprintf(ruta_manifiesto, sizeof(ruta_manifiesto), "%s/.ultimo_respaldo", destino_base);
    FILE *f = fopen(ruta_manifiesto, "r");
    if (!f) return 0;
    long valor = 0;
    fscanf(f, "%ld", &valor);
    fclose(f);
    return (time_t) valor;
}

static void guardar_ultimo_respaldo(const char *destino_base, time_t t) {
    char ruta_manifiesto[1024];
    snprintf(ruta_manifiesto, sizeof(ruta_manifiesto), "%s/.ultimo_respaldo", destino_base);
    FILE *f = fopen(ruta_manifiesto, "w");
    if (!f) return;
    fprintf(f, "%ld", (long) t);
    fclose(f);
}

int respaldos_crear_incremental(const char *origen, const char *destino_base) {
    mkdir(destino_base, 0755);

    time_t ultimo = leer_ultimo_respaldo(destino_base);
    time_t ahora = time(NULL);

    char version[64];
    utils_timestamp(version, sizeof(version));

    char destino_version[1024];
    snprintf(destino_version, sizeof(destino_version), "%s/%s", destino_base, version);

    utils_titulo("Respaldo incremental");
    printf("Origen:  %s\n", origen);
    printf("Versión: %s\n", version);
    printf(ultimo == 0 ? "Modo: respaldo completo (primera vez)\n"
                        : "Modo: incremental (solo cambios)\n");

    int copiados = copiar_directorio_recursivo(origen, destino_version, ultimo);
    guardar_ultimo_respaldo(destino_base, ahora);

    char msg[256];
    snprintf(msg, sizeof(msg), "Respaldo %s creado, %d archivo(s) copiados", version, copiados);
    utils_log("respaldos", msg);

    printf(COLOR_OK "Respaldo completado: %d archivo(s) copiados.\n" COLOR_RESET, copiados);
    return copiados;
}

void respaldos_listar_versiones(const char *destino_base) {
    DIR *d = opendir(destino_base);
    if (!d) { printf(COLOR_INFO "No hay respaldos todavía.\n" COLOR_RESET); return; }

    utils_titulo("Versiones de respaldo disponibles");
    struct dirent *entrada;
    while ((entrada = readdir(d)) != NULL) {
        if (entrada->d_name[0] == '.') continue;
        printf("- %s\n", entrada->d_name);
    }
    closedir(d);
}

int respaldos_restaurar(const char *destino_base, const char *version, const char *ruta_restauracion) {
    char origen_version[1024];
    snprintf(origen_version, sizeof(origen_version), "%s/%s", destino_base, version);

    struct stat st;
    if (stat(origen_version, &st) != 0) {
        printf(COLOR_ERROR "La versión '%s' no existe.\n" COLOR_RESET, version);
        return -1;
    }

    int restaurados = copiar_directorio_recursivo(origen_version, ruta_restauracion, 0);
    printf(COLOR_OK "Restauración completada: %d archivo(s) restaurados en %s\n" COLOR_RESET,
           restaurados, ruta_restauracion);
    return restaurados;
}