#ifndef RESPALDOS_H
#define RESPALDOS_H

typedef struct {
    char nombres[64][128];
    int total;
} ListaVersiones;

typedef struct {
    char items[64][512];
    int total;
} ColaSnapshot;

int  respaldos_crear_incremental(const char *origen, const char *destino_base);
void respaldos_listar_versiones(const char *destino_base);
int  respaldos_restaurar(const char *destino_base, const char *version, const char *ruta_restauracion);

/* Cola de descargas */
int  respaldos_cola_agregar(const char *url_o_ruta);
void respaldos_cola_procesar(void);
void respaldos_cola_mostrar(void);

/* Variantes para GUI */
ListaVersiones respaldos_obtener_versiones(const char *destino_base);
ColaSnapshot respaldos_cola_obtener(void);
/* Procesa la cola invocando on_progreso(item, porcentaje, datos_usuario) en cada paso,
   permitiendo que la GUI actualice una barra de progreso sin bloquear la interfaz. */
void respaldos_cola_procesar_cb(void (*on_progreso)(const char *item, int pct, void *ud), void *ud);

#endif
