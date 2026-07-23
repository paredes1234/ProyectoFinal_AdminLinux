#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include "../include/procesos.h"
#include "../include/utils.h"

#define MAX_PROCESOS 2048

/* Lee tiempo total de CPU del sistema desde /proc/stat */
static unsigned long long tiempo_cpu_total(void) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0;
    char etiqueta[16];
    unsigned long long user, nice, sys, idle, iowait, irq, softirq, steal;
    fscanf(f, "%s %llu %llu %llu %llu %llu %llu %llu %llu",
           etiqueta, &user, &nice, &sys, &idle, &iowait, &irq, &softirq, &steal);
    fclose(f);
    return user + nice + sys + idle + iowait + irq + softirq + steal;
}

/* Lee utime+stime de un proceso desde /proc/[pid]/stat */
static unsigned long long tiempo_cpu_proceso(int pid) {
    char ruta[64];
    snprintf(ruta, sizeof(ruta), "/proc/%d/stat", pid);
    FILE *f = fopen(ruta, "r");
    if (!f) return 0;

    char buffer[1024];
    if (!fgets(buffer, sizeof(buffer), f)) { fclose(f); return 0; }
    fclose(f);

    /* El nombre del proceso está entre paréntesis y puede contener espacios,
       por eso ubicamos el último ')' antes de parsear los campos numéricos */
    char *cierre = strrchr(buffer, ')');
    if (!cierre) return 0;

    unsigned long long utime = 0, stime = 0;
    sscanf(cierre + 2,
           "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu",
           &utime, &stime);
    return utime + stime;
}

int procesos_obtener_info(int pid, InfoProceso *info) {
    char ruta[64], buffer[1024];
    FILE *f;

    memset(info, 0, sizeof(InfoProceso));
    info->pid = pid;

    /* /proc/[pid]/status: nombre, estado, ppid, memoria */
    snprintf(ruta, sizeof(ruta), "/proc/%d/status", pid);
    f = fopen(ruta, "r");
    if (!f) return -1;

    while (fgets(buffer, sizeof(buffer), f)) {
        if (strncmp(buffer, "Name:", 5) == 0)
            sscanf(buffer, "Name:\t%255[^\n]", info->nombre);
        else if (strncmp(buffer, "State:", 6) == 0)
            sscanf(buffer, "State:\t%15[^\n]", info->estado);
        else if (strncmp(buffer, "PPid:", 5) == 0)
            sscanf(buffer, "PPid:\t%d", &info->ppid);
        else if (strncmp(buffer, "VmRSS:", 6) == 0)
            sscanf(buffer, "VmRSS:\t%ld", &info->mem_kb);
    }
    fclose(f);
    return 0;
}

static double calcular_cpu_pct(int pid) {
    unsigned long long cpu_antes = tiempo_cpu_proceso(pid);
    unsigned long long total_antes = tiempo_cpu_total();
    usleep(150000); /* muestreo de 150ms para estimar uso de CPU */
    unsigned long long cpu_despues = tiempo_cpu_proceso(pid);
    unsigned long long total_despues = tiempo_cpu_total();

    unsigned long long delta_cpu = cpu_despues - cpu_antes;
    unsigned long long delta_total = total_despues - total_antes;
    if (delta_total == 0) return 0.0;

    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    return (double) delta_cpu / (double) delta_total * 100.0 * ncpu;
}

void procesos_listar(void) {
    DIR *d = opendir("/proc");
    if (!d) { printf(COLOR_ERROR "No se pudo abrir /proc\n" COLOR_RESET); return; }

    utils_titulo("Procesos en ejecución");
    printf("%-8s %-8s %-20s %-10s %-10s\n", "PID", "PPID", "NOMBRE", "ESTADO", "MEM(KB)");
    printf("--------------------------------------------------------\n");

    struct dirent *entrada;
    while ((entrada = readdir(d)) != NULL) {
        if (!isdigit((unsigned char) entrada->d_name[0])) continue;
        int pid = atoi(entrada->d_name);
        InfoProceso info;
        if (procesos_obtener_info(pid, &info) == 0) {
            printf("%-8d %-8d %-20s %-10s %-10ld\n",
                   info.pid, info.ppid, info.nombre, info.estado, info.mem_kb);
        }
    }
    closedir(d);
}

void procesos_buscar_por_nombre(const char *nombre) {
    DIR *d = opendir("/proc");
    if (!d) return;

    utils_titulo("Resultados de búsqueda");
    int encontrados = 0;
    struct dirent *entrada;
    while ((entrada = readdir(d)) != NULL) {
        if (!isdigit((unsigned char) entrada->d_name[0])) continue;
        int pid = atoi(entrada->d_name);
        InfoProceso info;
        if (procesos_obtener_info(pid, &info) == 0 && strstr(info.nombre, nombre)) {
            printf("PID: %-8d PPID: %-8d NOMBRE: %-20s ESTADO: %s\n",
                   info.pid, info.ppid, info.nombre, info.estado);
            encontrados++;
        }
    }
    closedir(d);
    if (!encontrados) printf(COLOR_INFO "No se encontraron coincidencias.\n" COLOR_RESET);
}

void procesos_mostrar_cpu_mem(int pid) {
    InfoProceso info;
    if (procesos_obtener_info(pid, &info) != 0) {
        printf(COLOR_ERROR "PID %d no encontrado.\n" COLOR_RESET, pid);
        return;
    }
    double cpu = calcular_cpu_pct(pid);
    char mem_fmt[32];
    utils_formatear_bytes(info.mem_kb * 1024, mem_fmt, sizeof(mem_fmt));
    printf("PID: %d | Nombre: %s | CPU: %.2f%% | Memoria: %s\n",
           info.pid, info.nombre, cpu, mem_fmt);
}

int procesos_finalizar(int pid) {
    if (kill(pid, SIGTERM) == 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Proceso %d finalizado (SIGTERM)", pid);
        utils_log("procesos", msg);
        printf(COLOR_OK "Proceso %d finalizado.\n" COLOR_RESET, pid);
        return 0;
    }
    perror("kill");
    return -1;
}

int procesos_suspender(int pid) {
    if (kill(pid, SIGSTOP) == 0) {
        printf(COLOR_OK "Proceso %d suspendido.\n" COLOR_RESET, pid);
        return 0;
    }
    perror("kill");
    return -1;
}

int procesos_reanudar(int pid) {
    if (kill(pid, SIGCONT) == 0) {
        printf(COLOR_OK "Proceso %d reanudado.\n" COLOR_RESET, pid);
        return 0;
    }
    perror("kill");
    return -1;
}

static int g_pids[MAX_PROCESOS];
static int g_ppids[MAX_PROCESOS];
static char g_nombres[MAX_PROCESOS][256];
static int g_total = 0;

static void arbol_imprimir_hijos(int padre, int nivel) {
    for (int i = 0; i < g_total; i++) {
        if (g_ppids[i] == padre) {
            for (int j = 0; j < nivel; j++) printf("  ");
            printf("└─ %s (PID %d)\n", g_nombres[i], g_pids[i]);
            arbol_imprimir_hijos(g_pids[i], nivel + 1);
        }
    }
}

void procesos_arbol(void) {
    DIR *d = opendir("/proc");
    if (!d) return;

    utils_titulo("Árbol de procesos (raíz: PID 1)");

    g_total = 0;
    struct dirent *entrada;
    while ((entrada = readdir(d)) != NULL && g_total < MAX_PROCESOS) {
        if (!isdigit((unsigned char) entrada->d_name[0])) continue;
        int pid = atoi(entrada->d_name);
        InfoProceso info;
        if (procesos_obtener_info(pid, &info) == 0) {
            g_pids[g_total] = info.pid;
            g_ppids[g_total] = info.ppid;
            strncpy(g_nombres[g_total], info.nombre, 255);
            g_total++;
        }
    }
    closedir(d);

    arbol_imprimir_hijos(1, 0);
}

/* ===================================================================
 * A partir de aquí: funciones que devuelven datos estructurados
 * (arreglos / cadenas) en lugar de imprimir directamente. Son usadas
 * por la interfaz gráfica (GTK), pero reutilizan la misma lógica de
 * lectura de /proc que las funciones CLI de arriba.
 * =================================================================== */

double procesos_calcular_cpu_pct(int pid) {
    return calcular_cpu_pct(pid);
}

ListaProcesos procesos_obtener_lista(void) {
    ListaProcesos lista = { NULL, 0 };
    DIR *d = opendir("/proc");
    if (!d) return lista;

    int capacidad = 256;
    lista.items = malloc(sizeof(InfoProceso) * capacidad);

    struct dirent *entrada;
    while ((entrada = readdir(d)) != NULL) {
        if (!isdigit((unsigned char) entrada->d_name[0])) continue;
        int pid = atoi(entrada->d_name);
        InfoProceso info;
        if (procesos_obtener_info(pid, &info) == 0) {
            if (lista.total >= capacidad) {
                capacidad *= 2;
                lista.items = realloc(lista.items, sizeof(InfoProceso) * capacidad);
            }
            lista.items[lista.total++] = info;
        }
    }
    closedir(d);
    return lista;
}

ListaProcesos procesos_buscar_lista(const char *nombre) {
    ListaProcesos completa = procesos_obtener_lista();
    ListaProcesos filtrada = { NULL, 0 };
    filtrada.items = malloc(sizeof(InfoProceso) * (completa.total > 0 ? completa.total : 1));

    for (int i = 0; i < completa.total; i++) {
        if (strstr(completa.items[i].nombre, nombre)) {
            filtrada.items[filtrada.total++] = completa.items[i];
        }
    }
    procesos_lista_liberar(&completa);
    return filtrada;
}

void procesos_lista_liberar(ListaProcesos *lista) {
    free(lista->items);
    lista->items = NULL;
    lista->total = 0;
}

/* Construye el árbol de procesos como texto (para mostrar en un diálogo GTK).
   El llamador debe liberar la cadena devuelta con free(). */
static void arbol_construir_texto(int padre, int nivel, char **buffer, size_t *tam, size_t *usado) {
    for (int i = 0; i < g_total; i++) {
        if (g_ppids[i] == padre) {
            char linea[320];
            int pos = 0;
            for (int j = 0; j < nivel && pos < 300; j++) { linea[pos++] = ' '; linea[pos++] = ' '; }
            pos += snprintf(linea + pos, sizeof(linea) - pos, "└─ %s (PID %d)\n", g_nombres[i], g_pids[i]);

            size_t len = strlen(linea);
            if (*usado + len + 1 > *tam) {
                *tam = (*tam + len + 1) * 2;
                *buffer = realloc(*buffer, *tam);
            }
            memcpy(*buffer + *usado, linea, len + 1);
            *usado += len;

            arbol_construir_texto(g_pids[i], nivel + 1, buffer, tam, usado);
        }
    }
}

char *procesos_arbol_texto(void) {
    DIR *d = opendir("/proc");
    if (!d) return strdup("No se pudo leer /proc\n");

    g_total = 0;
    struct dirent *entrada;
    while ((entrada = readdir(d)) != NULL && g_total < MAX_PROCESOS) {
        if (!isdigit((unsigned char) entrada->d_name[0])) continue;
        int pid = atoi(entrada->d_name);
        InfoProceso info;
        if (procesos_obtener_info(pid, &info) == 0) {
            g_pids[g_total] = info.pid;
            g_ppids[g_total] = info.ppid;
            strncpy(g_nombres[g_total], info.nombre, 255);
            g_total++;
        }
    }
    closedir(d);

    size_t tam = 4096, usado = 0;
    char *buffer = malloc(tam);
    buffer[0] = '\0';
    arbol_construir_texto(1, 0, &buffer, &tam, &usado);
    return buffer;
}