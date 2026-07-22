#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/comandos.h"
#include "../include/utils.h"

#define ARCHIVO_HISTORIAL "logs/historial_comandos.log"

static void guardar_en_historial(const char *cmd) {
    FILE *f = fopen(ARCHIVO_HISTORIAL, "a");
    if (!f) return;
    char ts[64];
    utils_timestamp(ts, sizeof(ts));
    fprintf(f, "[%s] %s\n", ts, cmd);
    fclose(f);
}

ResultadoComando comandos_ejecutar_capturar(const char *cmd) {
    ResultadoComando resultado = { strdup(""), strdup(""), -1 };

    int pipe_out[2], pipe_err[2];
    if (pipe(pipe_out) == -1 || pipe(pipe_err) == -1) {
        perror("pipe");
        return resultado;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return resultado;
    }

    if (pid == 0) {
        dup2(pipe_out[1], STDOUT_FILENO);
        dup2(pipe_err[1], STDERR_FILENO);
        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_err[0]); close(pipe_err[1]);

        execl("/bin/sh", "sh", "-c", cmd, (char *) NULL);
        _exit(127);
    }

    close(pipe_out[1]);
    close(pipe_err[1]);

    char buffer[4096];
    ssize_t leidos;

    size_t cap_out = 4096, usado_out = 0;
    free(resultado.salida_estandar);
    resultado.salida_estandar = malloc(cap_out);
    resultado.salida_estandar[0] = '\0';

    while ((leidos = read(pipe_out[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[leidos] = '\0';
        if (usado_out + (size_t) leidos + 1 > cap_out) {
            cap_out = (cap_out + leidos + 1) * 2;
            resultado.salida_estandar = realloc(resultado.salida_estandar, cap_out);
        }
        memcpy(resultado.salida_estandar + usado_out, buffer, leidos + 1);
        usado_out += leidos;
    }
    close(pipe_out[0]);

    size_t cap_err = 4096, usado_err = 0;
    free(resultado.salida_error);
    resultado.salida_error = malloc(cap_err);
    resultado.salida_error[0] = '\0';

    while ((leidos = read(pipe_err[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[leidos] = '\0';
        if (usado_err + (size_t) leidos + 1 > cap_err) {
            cap_err = (cap_err + leidos + 1) * 2;
            resultado.salida_error = realloc(resultado.salida_error, cap_err);
        }
        memcpy(resultado.salida_error + usado_err, buffer, leidos + 1);
        usado_err += leidos;
    }
    close(pipe_err[0]);

    int estado;
    waitpid(pid, &estado, 0);
    resultado.codigo_salida = WIFEXITED(estado) ? WEXITSTATUS(estado) : -1;

    guardar_en_historial(cmd);
    return resultado;
}

void comandos_ejecutar(const char *cmd) {
    ResultadoComando r = comandos_ejecutar_capturar(cmd);

    printf(COLOR_OK "--- Salida estándar ---\n" COLOR_RESET);
    printf("%s", r.salida_estandar);

    printf(COLOR_ERROR "--- Salida de error ---\n" COLOR_RESET);
    printf("%s", r.salida_error);

    printf(COLOR_INFO "Código de salida: %d\n" COLOR_RESET, r.codigo_salida);

    free(r.salida_estandar);
    free(r.salida_error);
}

void comandos_mostrar_historial(void) {
    FILE *f = fopen(ARCHIVO_HISTORIAL, "r");
    if (!f) { printf(COLOR_INFO "No hay historial todavía.\n" COLOR_RESET); return; }

    utils_titulo("Historial de comandos");
    char linea[512];
    while (fgets(linea, sizeof(linea), f)) {
        printf("%s", linea);
    }
    fclose(f);
}

char *comandos_obtener_historial_texto(void) {
    FILE *f = fopen(ARCHIVO_HISTORIAL, "r");
    if (!f) return strdup("No hay historial todavía.\n");

    size_t cap = 4096, usado = 0;
    char *resultado = malloc(cap);
    resultado[0] = '\0';

    char linea[512];
    while (fgets(linea, sizeof(linea), f)) {
        size_t len = strlen(linea);
        if (usado + len + 1 > cap) {
            cap = (cap + len + 1) * 2;
            resultado = realloc(resultado, cap);
        }
        memcpy(resultado + usado, linea, len + 1);
        usado += len;
    }
    fclose(f);
    return resultado;
}


