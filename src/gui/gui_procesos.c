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

/* Devuelve el PID seleccionado en la tabla, o -1 si no hay selección */
static int obtener_pid_seleccionado(void) {
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_treeview));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) return -1;
    int pid;
    gtk_tree_model_get(model, &iter, COL_PID, &pid, -1);
    return pid;
}

static void on_finalizar_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    int pid = obtener_pid_seleccionado();
    if (pid < 0) { gui_mostrar_error(g_ventana, "Selecciona un proceso de la lista."); return; }
    if (procesos_finalizar(pid) == 0) {
        gui_mostrar_info(g_ventana, "Proceso finalizado", "El proceso fue finalizado correctamente.");
        on_actualizar_clicked(NULL, NULL);
    } else {
        gui_mostrar_error(g_ventana, "No se pudo finalizar el proceso (¿permisos insuficientes?).");
    }
}

static void on_suspender_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    int pid = obtener_pid_seleccionado();
    if (pid < 0) { gui_mostrar_error(g_ventana, "Selecciona un proceso de la lista."); return; }
    if (procesos_suspender(pid) == 0) {
        on_actualizar_clicked(NULL, NULL);
    } else {
        gui_mostrar_error(g_ventana, "No se pudo suspender el proceso.");
    }
}

static void on_reanudar_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    int pid = obtener_pid_seleccionado();
    if (pid < 0) { gui_mostrar_error(g_ventana, "Selecciona un proceso de la lista."); return; }
    if (procesos_reanudar(pid) == 0) {
        on_actualizar_clicked(NULL, NULL);
    } else {
        gui_mostrar_error(g_ventana, "No se pudo reanudar el proceso.");
    }
}

static void on_cpu_mem_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    int pid = obtener_pid_seleccionado();
    if (pid < 0) { gui_mostrar_error(g_ventana, "Selecciona un proceso de la lista."); return; }

    InfoProceso info;
    if (procesos_obtener_info(pid, &info) != 0) {
        gui_mostrar_error(g_ventana, "El proceso ya no existe.");
        return;
    }
    double cpu = procesos_calcular_cpu_pct(pid); /* toma ~150ms en muestrear */
    char mensaje[512];
    snprintf(mensaje, sizeof(mensaje),
             "PID: %d\nNombre: %s\nCPU: %.2f%%\nMemoria (RSS): %ld KB",
             info.pid, info.nombre, cpu, info.mem_kb);
    gui_mostrar_info(g_ventana, "Uso de CPU y memoria", mensaje);
}

static void on_arbol_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    char *texto = procesos_arbol_texto();
    gui_mostrar_texto_largo(g_ventana, "Árbol de procesos (raíz: PID 1)", texto);
    free(texto);
}