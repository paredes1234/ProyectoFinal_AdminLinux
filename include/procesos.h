#ifndef PROCESOS_H
#define PROCESOS_H

typedef struct {
    int pid;
    int ppid;
    char nombre[256];
    char estado[16];
    long mem_kb;
    double cpu_pct;
} InfoProceso;

typedef struct {
    InfoProceso *items;
    int total;
} ListaProcesos;

void procesos_listar(void);
void procesos_buscar_por_nombre(const char *nombre);
int  procesos_obtener_info(int pid, InfoProceso *info);
void procesos_mostrar_cpu_mem(int pid);
int  procesos_finalizar(int pid);
int  procesos_suspender(int pid);
int  procesos_reanudar(int pid);
void procesos_arbol(void);

/* Variantes que devuelven datos estructurados (usadas por la GUI) */
double procesos_calcular_cpu_pct(int pid);
ListaProcesos procesos_obtener_lista(void);
ListaProcesos procesos_buscar_lista(const char *nombre);
void procesos_lista_liberar(ListaProcesos *lista);
char *procesos_arbol_texto(void); /* el llamador debe hacer free() */

#endif