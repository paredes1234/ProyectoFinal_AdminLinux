#ifndef BASH_ANALYZER_H
#define BASH_ANALYZER_H

typedef struct {
    int total_lineas;
    int total_vars;
    int num_for, num_while, num_until, num_if, num_func;
    char *detalle; /* texto línea por línea con hallazgos; el llamador debe hacer free() */
} ResultadoAnalisisBash;

void bash_analizar_script(const char *ruta_script);

/* Variante que devuelve el resultado estructurado (usada por la GUI) */
ResultadoAnalisisBash bash_analizar_script_datos(const char *ruta_script);

#endif
