#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/bash_analyzer.h"
#include "../include/utils.h"

#define MAX_VARS 256

static int es_asignacion_variable(const char *linea, char *nombre_var, int tam) {
    /* Busca patrón NOMBRE=valor al inicio de línea (sin espacios antes del =) */
    int i = 0;
    while (isspace((unsigned char) linea[i])) i++;

    int inicio = i;
    if (!isalpha((unsigned char) linea[i]) && linea[i] != '_') return 0;

    while (isalnum((unsigned char) linea[i]) || linea[i] == '_') i++;
    int fin = i;
    if (fin == inicio) return 0;
    if (linea[i] != '=') return 0;
    /* Evitar confundir con comparaciones == o !=, [ x = y ] */
    if (linea[i + 1] == '=') return 0;

    int len = fin - inicio;
    if (len >= tam) len = tam - 1;
    strncpy(nombre_var, linea + inicio, len);
    nombre_var[len] = '\0';
    return 1;
}

static int ya_registrada(char vars[][128], int total, const char *nombre) {
    for (int i = 0; i < total; i++) {
        if (strcmp(vars[i], nombre) == 0) return 1;
    }
    return 0;
}

/* Realiza el análisis léxico del script y devuelve un resultado estructurado.
   El llamador debe liberar resultado.detalle con free(). */
ResultadoAnalisisBash bash_analizar_script_datos(const char *ruta_script) {
    ResultadoAnalisisBash r;
    memset(&r, 0, sizeof(r));

    size_t cap = 4096, usado = 0;
    r.detalle = malloc(cap);
    r.detalle[0] = '\0';

    FILE *f = fopen(ruta_script, "r");
    if (!f) {
        snprintf(r.detalle, cap, "No se pudo abrir el script: %s\n", ruta_script);
        return r;
    }

    char linea[1024];
    char variables[MAX_VARS][128];
    int total_vars = 0;
    int num_linea = 0;

    while (fgets(linea, sizeof(linea), f)) {
        num_linea++;

        char *sin_espacios = linea;
        while (isspace((unsigned char) *sin_espacios)) sin_espacios++;
        if (sin_espacios[0] == '#') continue;

        char nota[256] = "";

        char nombre_var[128];
        if (es_asignacion_variable(linea, nombre_var, sizeof(nombre_var))) {
            if (!ya_registrada(variables, total_vars, nombre_var) && total_vars < MAX_VARS) {
                strncpy(variables[total_vars], nombre_var, 127);
                total_vars++;
                snprintf(nota, sizeof(nota), "[Línea %3d] Variable declarada: %s\n", num_linea, nombre_var);
            }
        }
        if (strstr(linea, "for ") || strstr(linea, "for(")) {
            r.num_for++;
            snprintf(nota + strlen(nota), sizeof(nota) - strlen(nota),
                     "[Línea %3d] Ciclo FOR detectado\n", num_linea);
        }
        if (strstr(linea, "while ") || strstr(linea, "while(")) {
            r.num_while++;
            snprintf(nota + strlen(nota), sizeof(nota) - strlen(nota),
                     "[Línea %3d] Ciclo WHILE detectado\n", num_linea);
        }
        if (strstr(linea, "until ")) {
            r.num_until++;
            snprintf(nota + strlen(nota), sizeof(nota) - strlen(nota),
                     "[Línea %3d] Ciclo UNTIL detectado\n", num_linea);
        }
        if (strstr(linea, "if ") || strstr(linea, "if[")) {
            r.num_if++;
        }
        if (strstr(linea, "function ") || strstr(linea, "() {")) {
            r.num_func++;
            snprintf(nota + strlen(nota), sizeof(nota) - strlen(nota),
                     "[Línea %3d] Función detectada\n", num_linea);
        }

        if (nota[0] != '\0') {
            size_t len = strlen(nota);
            if (usado + len + 1 > cap) {
                cap = (cap + len + 1) * 2;
                r.detalle = realloc(r.detalle, cap);
            }
            memcpy(r.detalle + usado, nota, len + 1);
            usado += len;
        }
    }
    fclose(f);

    r.total_lineas = num_linea;
    r.total_vars = total_vars;

    char msg[256];
    snprintf(msg, sizeof(msg), "Analizado %s: %d vars, %d ciclos",
             ruta_script, r.total_vars, r.num_for + r.num_while + r.num_until);
    utils_log("bash_analyzer", msg);

    return r;
}

/* Versión CLI: reutiliza el análisis estructurado y lo imprime en pantalla */
void bash_analizar_script(const char *ruta_script) {
    utils_titulo("Análisis de script Bash");
    printf("Archivo: %s\n\n", ruta_script);

    ResultadoAnalisisBash r = bash_analizar_script_datos(ruta_script);
    printf("  %s", r.detalle);

    utils_titulo("Resumen");
    printf("Total de líneas analizadas: %d\n", r.total_lineas);
    printf("Variables únicas detectadas: %d\n", r.total_vars);
    printf("Ciclos FOR:    %d\n", r.num_for);
    printf("Ciclos WHILE:  %d\n", r.num_while);
    printf("Ciclos UNTIL:  %d\n", r.num_until);
    printf("Condicionales IF: %d\n", r.num_if);
    printf("Funciones:     %d\n", r.num_func);

    free(r.detalle);
}
