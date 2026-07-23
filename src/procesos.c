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
