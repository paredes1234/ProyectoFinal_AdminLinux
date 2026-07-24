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

/* ---------- Cola de descargas (con progreso simulado) ---------- */

static char g_cola[MAX_COLA][512];
static int g_cola_tam = 0;

int respaldos_cola_agregar(const char *url_o_ruta) {
    if (g_cola_tam >= MAX_COLA) {
        printf(COLOR_ERROR "Cola llena.\n" COLOR_RESET);
        return -1;
    }
    strncpy(g_cola[g_cola_tam], url_o_ruta, 511);
    g_cola_tam++;
    printf(COLOR_OK "Agregado a la cola: %s\n" COLOR_RESET, url_o_ruta);
    return 0;
}

void respaldos_cola_mostrar(void) {
    utils_titulo("Cola de descargas");
    if (g_cola_tam == 0) { printf(COLOR_INFO "La cola está vacía.\n" COLOR_RESET); return; }
    for (int i = 0; i < g_cola_tam; i++) {
        printf("%d. %s\n", i + 1, g_cola[i]);
    }
}

/* Procesa la cola emitiendo eventos de progreso por cada elemento.
   Aquí se simula el avance; en una versión real se integraría con
   una descarga real (p. ej. usando sockets o una librería HTTP). */
void respaldos_cola_procesar(void) {
    if (g_cola_tam == 0) {
        printf(COLOR_INFO "No hay elementos en la cola.\n" COLOR_RESET);
        return;
    }

    utils_titulo("Procesando cola de descargas");
    for (int i = 0; i < g_cola_tam; i++) {
        printf("Descargando: %s\n", g_cola[i]);
        for (int p = 0; p <= 100; p += 20) {
            printf("\r  Progreso: [%-20s] %d%%", "", p);
            fflush(stdout);
            /* Barra de progreso visual */
            printf("\r  Progreso: [");
            int lleno = p / 5;
            for (int b = 0; b < 20; b++) printf(b < lleno ? "#" : " ");
            printf("] %d%%", p);
            fflush(stdout);
            usleep(80000);
        }
        printf("\n" COLOR_OK "  Completado.\n" COLOR_RESET);

        char msg[600];
        snprintf(msg, sizeof(msg), "Descarga completada: %s", g_cola[i]);
        utils_log("respaldos", msg);
    }
    g_cola_tam = 0;
}

/* ===================================================================
 * Variantes para GUI
 * =================================================================== */

ListaVersiones respaldos_obtener_versiones(const char *destino_base) {
    ListaVersiones lista = { .total = 0 };
    DIR *d = opendir(destino_base);
    if (!d) return lista;

    struct dirent *entrada;
    while ((entrada = readdir(d)) != NULL && lista.total < 64) {
        if (entrada->d_name[0] == '.') continue;
        strncpy(lista.nombres[lista.total], entrada->d_name, 127);
        lista.nombres[lista.total][127] = '\0';
        lista.total++;
    }
    closedir(d);
    return lista;
}

ColaSnapshot respaldos_cola_obtener(void) {
    ColaSnapshot snap;
    snap.total = g_cola_tam;
    for (int i = 0; i < g_cola_tam; i++) {
        strncpy(snap.items[i], g_cola[i], 511);
        snap.items[i][511] = '\0';
    }
    return snap;
}

void respaldos_cola_procesar_cb(void (*on_progreso)(const char *item, int pct, void *ud), void *ud) {
    for (int i = 0; i < g_cola_tam; i++) {
        for (int p = 0; p <= 100; p += 10) {
            if (on_progreso) on_progreso(g_cola[i], p, ud);
            usleep(60000);
        }
        char msg[600];
        snprintf(msg, sizeof(msg), "Descarga completada: %s", g_cola[i]);
        utils_log("respaldos", msg);
    }
    g_cola_tam = 0;
}
