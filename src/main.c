#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/procesos.h"
#include "../include/archivos.h"
#include "../include/comandos.h"
#include "../include/respaldos.h"
#include "../include/bash_analyzer.h"
#include "../include/utils.h"

#define LINEA_MAX 512

static void leer_linea(char *buffer, int tam) {
    if (fgets(buffer, tam, stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
    }
}

static void menu_procesos(void) {
    char buffer[LINEA_MAX];
    int opcion;
    do {
        utils_titulo("ADMINISTRADOR DE TAREAS");
        printf("1. Listar procesos\n");
        printf("2. Buscar proceso por nombre\n");
        printf("3. Ver CPU y memoria de un PID\n");
        printf("4. Finalizar proceso\n");
        printf("5. Suspender proceso\n");
        printf("6. Reanudar proceso\n");
        printf("7. Ver árbol de procesos\n");
        printf("0. Volver al menú principal\n");
        printf("Opción: ");
        leer_linea(buffer, sizeof(buffer));
        opcion = atoi(buffer);

        switch (opcion) {
            case 1: procesos_listar(); break;
            case 2:
                printf("Nombre a buscar: ");
                leer_linea(buffer, sizeof(buffer));
                procesos_buscar_por_nombre(buffer);
                break;
            case 3:
                printf("PID: ");
                leer_linea(buffer, sizeof(buffer));
                procesos_mostrar_cpu_mem(atoi(buffer));
                break;
            case 4:
                printf("PID a finalizar: ");
                leer_linea(buffer, sizeof(buffer));
                procesos_finalizar(atoi(buffer));
                break;
            case 5:
                printf("PID a suspender: ");
                leer_linea(buffer, sizeof(buffer));
                procesos_suspender(atoi(buffer));
                break;
            case 6:
                printf("PID a reanudar: ");
                leer_linea(buffer, sizeof(buffer));
                procesos_reanudar(atoi(buffer));
                break;
            case 7: procesos_arbol(); break;
        }
    } while (opcion != 0);
}
