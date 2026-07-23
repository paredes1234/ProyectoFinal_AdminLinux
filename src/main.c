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

static void menu_archivos(void) {
    char buffer[LINEA_MAX], b2[LINEA_MAX];
    int opcion;
    do {
        utils_titulo("SHELL DE ARCHIVOS");
        printf("1. Listar archivos de un directorio\n");
        printf("2. Copiar archivo o carpeta\n");
        printf("3. Mover archivo\n");
        printf("4. Eliminar archivo/directorio\n");
        printf("5. Buscar archivos (recursivo)\n");
        printf("6. Ver estadísticas de un archivo\n");
        printf("0. Volver al menú principal\n");
        printf("Opción: ");
        leer_linea(buffer, sizeof(buffer));
        opcion = atoi(buffer);

        switch (opcion) {
            case 1:
                printf("Ruta del directorio: ");
                leer_linea(buffer, sizeof(buffer));
                archivos_listar(buffer);
                break;
            case 2:
                printf("Origen: "); leer_linea(buffer, sizeof(buffer));
                printf("Destino: "); leer_linea(b2, sizeof(b2));
                archivos_copiar(buffer, b2);
                break;
            case 3:
                printf("Origen: "); leer_linea(buffer, sizeof(buffer));
                printf("Destino: "); leer_linea(b2, sizeof(b2));
                archivos_mover(buffer, b2);
                break;
            case 4:
                printf("Ruta a eliminar: ");
                leer_linea(buffer, sizeof(buffer));
                archivos_eliminar(buffer);
                break;
            case 5:
                printf("Directorio base: "); leer_linea(buffer, sizeof(buffer));
                printf("Patrón a buscar: "); leer_linea(b2, sizeof(b2));
                archivos_buscar(buffer, b2);
                break;
            case 6:
                printf("Ruta: ");
                leer_linea(buffer, sizeof(buffer));
                archivos_estadisticas(buffer);
                break;
        }
    } while (opcion != 0);
}

static void menu_comandos(void) {
    char buffer[LINEA_MAX];
    int opcion;
    do {
        utils_titulo("COMANDOS LINUX");
        printf("1. Ejecutar comando\n");
        printf("2. Ver historial\n");
        printf("3. Limpiar historial\n");
        printf("0. Volver al menú principal\n");
        printf("Opción: ");
        leer_linea(buffer, sizeof(buffer));
        opcion = atoi(buffer);

        switch (opcion) {
            case 1:
                printf("Comando: ");
                leer_linea(buffer, sizeof(buffer));
                comandos_ejecutar(buffer);
                break;
            case 2: comandos_mostrar_historial(); break;
            case 3: comandos_limpiar_historial(); break;
        }
    } while (opcion != 0);
}