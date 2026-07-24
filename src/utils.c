#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../include/utils.h"

void utils_titulo(const char *texto) {
    printf(COLOR_TITULO "\n=== %s ===" COLOR_RESET "\n", texto);
}

void utils_formatear_bytes(long bytes, char *salida, int tam) {
    const char *unidades[] = {"B", "KB", "MB", "GB", "TB"};
    double valor = (double) bytes;
    int i = 0;
    while (valor >= 1024.0 && i < 4) {
        valor /= 1024.0;
        i++;
    }
    snprintf(salida, tam, "%.2f %s", valor, unidades[i]);
}

void utils_timestamp(char *salida, int tam) {
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    strftime(salida, tam, "%Y-%m-%d_%H-%M-%S", t);
}

void utils_log(const char *modulo, const char *mensaje) {
    FILE *f = fopen("logs/admin.log", "a");
    if (!f) return;
    char ts[64];
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", t);
    fprintf(f, "[%s] (%s) %s\n", ts, modulo, mensaje);
    fclose(f);
}
