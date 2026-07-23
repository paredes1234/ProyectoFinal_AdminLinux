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

/* ---------------- Construcción de la pestaña ---------------- */

static GtkWidget *crear_seccion_respaldos(void) {
    GtkWidget *marco = gtk_frame_new("Respaldos incrementales");
    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(caja), 10);

    GtkWidget *fila1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    g_entrada_origen = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_entrada_origen), "Directorio origen");
    gtk_widget_set_hexpand(g_entrada_origen, TRUE);
    g_entrada_destino = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_entrada_destino), "Directorio de respaldos");
    gtk_widget_set_hexpand(g_entrada_destino, TRUE);
    GtkWidget *btn_crear = gtk_button_new_with_label("Crear respaldo");
    gtk_box_pack_start(GTK_BOX(fila1), g_entrada_origen, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(fila1), g_entrada_destino, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(fila1), btn_crear, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), fila1, FALSE, FALSE, 0);

    GtkWidget *fila2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *btn_listar_versiones = gtk_button_new_with_label("Listar versiones");
    g_combo_versiones = gtk_combo_box_text_new();
    gtk_widget_set_hexpand(g_combo_versiones, TRUE);
    g_entrada_restauracion = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_entrada_restauracion), "Ruta donde restaurar");
    gtk_widget_set_hexpand(g_entrada_restauracion, TRUE);
    GtkWidget *btn_restaurar = gtk_button_new_with_label("Restaurar");
    gtk_box_pack_start(GTK_BOX(fila2), btn_listar_versiones, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(fila2), g_combo_versiones, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(fila2), g_entrada_restauracion, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(fila2), btn_restaurar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), fila2, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(marco), caja);

    g_signal_connect(btn_crear, "clicked", G_CALLBACK(on_crear_respaldo_clicked), NULL);
    g_signal_connect(btn_listar_versiones, "clicked", G_CALLBACK(on_listar_versiones_clicked), NULL);
    g_signal_connect(btn_restaurar, "clicked", G_CALLBACK(on_restaurar_clicked), NULL);

    return marco;
}

static GtkWidget *crear_seccion_bash(void) {
    GtkWidget *marco = gtk_frame_new("Análisis de scripts Bash");
    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(caja), 10);

    GtkWidget *fila = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    g_entrada_script = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_entrada_script), "Ruta del script .sh");
    gtk_widget_set_hexpand(g_entrada_script, TRUE);
    GtkWidget *btn_analizar = gtk_button_new_with_label("Analizar");
    gtk_box_pack_start(GTK_BOX(fila), g_entrada_script, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(fila), btn_analizar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), fila, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll, -1, 160);
    GtkWidget *vista = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(vista), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(vista), TRUE);
    g_buffer_bash = gtk_text_view_get_buffer(GTK_TEXT_VIEW(vista));
    gtk_container_add(GTK_CONTAINER(scroll), vista);
    gtk_box_pack_start(GTK_BOX(caja), scroll, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(marco), caja);
    g_signal_connect(btn_analizar, "clicked", G_CALLBACK(on_analizar_bash_clicked), NULL);
    return marco;
}

static GtkWidget *crear_seccion_cola(void) {
    GtkWidget *marco = gtk_frame_new("Cola de descargas");
    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(caja), 10);

    GtkWidget *fila = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    g_entrada_cola = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_entrada_cola), "URL o ruta a encolar");
    gtk_widget_set_hexpand(g_entrada_cola, TRUE);
    GtkWidget *btn_agregar = gtk_button_new_with_label("Agregar");
    GtkWidget *btn_procesar = gtk_button_new_with_label("Procesar cola");
    gtk_box_pack_start(GTK_BOX(fila), g_entrada_cola, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(fila), btn_agregar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(fila), btn_procesar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), fila, FALSE, FALSE, 0);

    g_store_cola = gtk_list_store_new(N_COLUMNAS_COLA, G_TYPE_STRING);
    g_treeview_cola = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_store_cola));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *columna = gtk_tree_view_column_new_with_attributes(
        "Elemento", renderer, "text", COL_COLA_ITEM, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_treeview_cola), columna);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll, -1, 120);
    gtk_container_add(GTK_CONTAINER(scroll), g_treeview_cola);
    gtk_box_pack_start(GTK_BOX(caja), scroll, TRUE, TRUE, 0);

    g_barra_progreso = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(g_barra_progreso), TRUE);
    gtk_box_pack_start(GTK_BOX(caja), g_barra_progreso, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(marco), caja);

    g_signal_connect(btn_agregar, "clicked", G_CALLBACK(on_agregar_cola_clicked), NULL);
    g_signal_connect(g_entrada_cola, "activate", G_CALLBACK(on_agregar_cola_clicked), NULL);
    g_signal_connect(btn_procesar, "clicked", G_CALLBACK(on_procesar_cola_clicked), NULL);

    return marco;
}

GtkWidget *gui_respaldos_crear_tab(GtkWidget *ventana_principal) {
    g_ventana = ventana_principal;

    GtkWidget *scroll_general = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_general),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(caja), 10);

    gtk_box_pack_start(GTK_BOX(caja), crear_seccion_respaldos(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), crear_seccion_bash(), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(caja), crear_seccion_cola(), FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(scroll_general), caja);
    return scroll_general;
}