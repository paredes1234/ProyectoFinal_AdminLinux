#include "../../include/gui_common.h"
#include "../../include/procesos.h"
#include <stdlib.h>

enum { COL_PID, COL_PPID, COL_NOMBRE, COL_ESTADO, COL_MEM, N_COLUMNAS_PROC };

static GtkWidget *g_ventana;
static GtkListStore *g_store;
static GtkWidget *g_treeview;
static GtkWidget *g_entrada_busqueda;
static GtkWidget *g_etiqueta_estado;

static void refrescar_desde_lista(ListaProcesos lista) {
    gtk_list_store_clear(g_store);
    GtkTreeIter iter;
    for (int i = 0; i < lista.total; i++) {
        InfoProceso *p = &lista.items[i];
        gtk_list_store_append(g_store, &iter);
        gtk_list_store_set(g_store, &iter,
            COL_PID, p->pid, COL_PPID, p->ppid,
            COL_NOMBRE, p->nombre, COL_ESTADO, p->estado,
            COL_MEM, p->mem_kb, -1);
    }
    char resumen[64];
    snprintf(resumen, sizeof(resumen), "%d proceso(s)", lista.total);
    gtk_label_set_text(GTK_LABEL(g_etiqueta_estado), resumen);
}

static void on_actualizar_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    ListaProcesos lista = procesos_obtener_lista();
    refrescar_desde_lista(lista);
    procesos_lista_liberar(&lista);
}

static void on_buscar_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    const char *texto = gtk_entry_get_text(GTK_ENTRY(g_entrada_busqueda));
    if (texto[0] == '\0') { on_actualizar_clicked(NULL, NULL); return; }
    ListaProcesos lista = procesos_buscar_lista(texto);
    refrescar_desde_lista(lista);
    procesos_lista_liberar(&lista);
}
