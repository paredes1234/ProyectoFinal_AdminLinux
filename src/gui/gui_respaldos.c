#include "../../include/gui_common.h"
#include "../../include/respaldos.h"
#include "../../include/bash_analyzer.h"
#include <stdlib.h>

static GtkWidget *g_ventana;

/* --- Respaldos --- */
static GtkWidget *g_entrada_origen;
static GtkWidget *g_entrada_destino;
static GtkWidget *g_combo_versiones;
static GtkWidget *g_entrada_restauracion;

/* --- Bash --- */
static GtkWidget *g_entrada_script;
static GtkTextBuffer *g_buffer_bash;

/* --- Cola de descargas --- */
static GtkListStore *g_store_cola;
static GtkWidget *g_treeview_cola;
static GtkWidget *g_entrada_cola;
static GtkWidget *g_barra_progreso;

enum { COL_COLA_ITEM, N_COLUMNAS_COLA };

/* ---------------- Respaldos ---------------- */

static void on_crear_respaldo_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    const char *origen = gtk_entry_get_text(GTK_ENTRY(g_entrada_origen));
    const char *destino = gtk_entry_get_text(GTK_ENTRY(g_entrada_destino));
    if (origen[0] == '\0' || destino[0] == '\0') {
        gui_mostrar_error(g_ventana, "Indica el directorio origen y el directorio de respaldos.");
        return;
    }
    int copiados = respaldos_crear_incremental(origen, destino);
    char mensaje[128];
    snprintf(mensaje, sizeof(mensaje), "Respaldo completado: %d archivo(s) copiados.", copiados);
    gui_mostrar_info(g_ventana, "Respaldo incremental", mensaje);
}

static void on_listar_versiones_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    const char *destino = gtk_entry_get_text(GTK_ENTRY(g_entrada_destino));
    if (destino[0] == '\0') {
        gui_mostrar_error(g_ventana, "Indica el directorio de respaldos.");
        return;
    }

    GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT(g_combo_versiones);
    gtk_combo_box_text_remove_all(combo);

    ListaVersiones lista = respaldos_obtener_versiones(destino);
    if (lista.total == 0) {
        gui_mostrar_info(g_ventana, "Versiones", "No hay respaldos todavía en esa carpeta.");
        return;
    }
    for (int i = 0; i < lista.total; i++) {
        gtk_combo_box_text_append_text(combo, lista.nombres[i]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
}

static void on_restaurar_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    const char *destino = gtk_entry_get_text(GTK_ENTRY(g_entrada_destino));
    char *version = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(g_combo_versiones));
    const char *ruta_restauracion = gtk_entry_get_text(GTK_ENTRY(g_entrada_restauracion));

    if (!version) {
        gui_mostrar_error(g_ventana, "Selecciona una versión de la lista (usa 'Listar versiones').");
        return;
    }
    if (ruta_restauracion[0] == '\0') {
        gui_mostrar_error(g_ventana, "Indica la ruta donde restaurar.");
        g_free(version);
        return;
    }

    int restaurados = respaldos_restaurar(destino, version, ruta_restauracion);
    char mensaje[160];
    snprintf(mensaje, sizeof(mensaje), "Restauración completada: %d archivo(s) restaurados.", restaurados);
    gui_mostrar_info(g_ventana, "Restaurar respaldo", mensaje);
    g_free(version);
}

/* ---------------- Análisis Bash ---------------- */

static void on_analizar_bash_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    const char *ruta = gtk_entry_get_text(GTK_ENTRY(g_entrada_script));
    if (ruta[0] == '\0') {
        gui_mostrar_error(g_ventana, "Indica la ruta de un script .sh");
        return;
    }

    ResultadoAnalisisBash r = bash_analizar_script_datos(ruta);

    GString *texto = g_string_new(NULL);
    g_string_append_printf(texto, "%s\n", r.detalle);
    g_string_append_printf(texto,
        "--- Resumen ---\n"
        "Líneas analizadas: %d\n"
        "Variables únicas: %d\n"
        "Ciclos FOR: %d\n"
        "Ciclos WHILE: %d\n"
        "Ciclos UNTIL: %d\n"
        "Condicionales IF: %d\n"
        "Funciones: %d\n",
        r.total_lineas, r.total_vars, r.num_for, r.num_while, r.num_until, r.num_if, r.num_func);

    gtk_text_buffer_set_text(g_buffer_bash, texto->str, -1);
    g_string_free(texto, TRUE);
    free(r.detalle);
}

/* ---------------- Cola de descargas ---------------- */

static void refrescar_cola_visual(void) {
    gtk_list_store_clear(g_store_cola);
    ColaSnapshot snap = respaldos_cola_obtener();
    GtkTreeIter iter;
    for (int i = 0; i < snap.total; i++) {
        gtk_list_store_append(g_store_cola, &iter);
        gtk_list_store_set(g_store_cola, &iter, COL_COLA_ITEM, snap.items[i], -1);
    }
}

static void on_agregar_cola_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    const char *texto = gtk_entry_get_text(GTK_ENTRY(g_entrada_cola));
    if (texto[0] == '\0') return;
    respaldos_cola_agregar(texto);
    gtk_entry_set_text(GTK_ENTRY(g_entrada_cola), "");
    refrescar_cola_visual();
}

/* Callback invocado por el core mientras procesa la cola; actualiza la barra
   de progreso y procesa eventos pendientes para no congelar la ventana. */
static void progreso_callback(const char *item, int pct, void *ud) {
    (void) ud;
    char etiqueta[560];
    snprintf(etiqueta, sizeof(etiqueta), "%s (%d%%)", item, pct);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_barra_progreso), etiqueta);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_barra_progreso), pct / 100.0);

    while (gtk_events_pending()) gtk_main_iteration();
}

static void on_procesar_cola_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    respaldos_cola_procesar_cb(progreso_callback, NULL);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_barra_progreso), "Completado");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_barra_progreso), 1.0);
    refrescar_cola_visual();
}