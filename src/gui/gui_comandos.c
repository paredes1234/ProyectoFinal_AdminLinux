#include "../../include/gui_common.h"
#include "../../include/comandos.h"
#include <stdlib.h>

static GtkWidget *g_ventana;
static GtkWidget *g_entrada_comando;
static GtkTextBuffer *g_buffer_salida;
static GtkTextTag *g_tag_ok, *g_tag_error, *g_tag_info;

static void on_ejecutar_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    const char *cmd = gtk_entry_get_text(GTK_ENTRY(g_entrada_comando));
    if (cmd[0] == '\0') return;

    ResultadoComando r = comandos_ejecutar_capturar(cmd);

    GtkTextIter fin;
    gtk_text_buffer_get_end_iter(g_buffer_salida, &fin);

    char linea_cmd[600];
    snprintf(linea_cmd, sizeof(linea_cmd), "$ %s\n", cmd);
    gtk_text_buffer_insert_with_tags(g_buffer_salida, &fin, linea_cmd, -1, g_tag_info, NULL);

    gtk_text_buffer_get_end_iter(g_buffer_salida, &fin);
    if (r.salida_estandar[0] != '\0')
        gtk_text_buffer_insert_with_tags(g_buffer_salida, &fin, r.salida_estandar, -1, g_tag_ok, NULL);

    gtk_text_buffer_get_end_iter(g_buffer_salida, &fin);
    if (r.salida_error[0] != '\0')
        gtk_text_buffer_insert_with_tags(g_buffer_salida, &fin, r.salida_error, -1, g_tag_error, NULL);

    gtk_text_buffer_get_end_iter(g_buffer_salida, &fin);
    char linea_estado[64];
    snprintf(linea_estado, sizeof(linea_estado), "[código de salida: %d]\n\n", r.codigo_salida);
    gtk_text_buffer_insert_with_tags(g_buffer_salida, &fin, linea_estado, -1, g_tag_info, NULL);

    free(r.salida_estandar);
    free(r.salida_error);

    gtk_entry_set_text(GTK_ENTRY(g_entrada_comando), "");
}

static void on_historial_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    char *historial = comandos_obtener_historial_texto();
    gui_mostrar_texto_largo(g_ventana, "Historial de comandos", historial);
    free(historial);
}

static void on_limpiar_historial_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    comandos_limpiar_historial();
    gui_mostrar_info(g_ventana, "Historial", "El historial fue limpiado.");
}

static void on_limpiar_pantalla_clicked(GtkButton *btn, gpointer data) {
    (void) btn; (void) data;
    gtk_text_buffer_set_text(g_buffer_salida, "", -1);
}

GtkWidget *gui_comandos_crear_tab(GtkWidget *ventana_principal) {
    g_ventana = ventana_principal;

    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(caja), 10);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);

    GtkWidget *vista_salida = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(vista_salida), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(vista_salida), TRUE);
    g_buffer_salida = gtk_text_view_get_buffer(GTK_TEXT_VIEW(vista_salida));

    g_tag_ok = gtk_text_buffer_create_tag(g_buffer_salida, "ok", "foreground", "#2e7d32", NULL);
    g_tag_error = gtk_text_buffer_create_tag(g_buffer_salida, "error", "foreground", "#c62828", NULL);
    g_tag_info = gtk_text_buffer_create_tag(g_buffer_salida, "info", "foreground", "#1565c0",
        "weight", PANGO_WEIGHT_BOLD, NULL);

    gtk_container_add(GTK_CONTAINER(scroll), vista_salida);
    gtk_box_pack_start(GTK_BOX(caja), scroll, TRUE, TRUE, 0);

    GtkWidget *barra_entrada = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    g_entrada_comando = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_entrada_comando), "Escribe un comando, ej: ls -la");
    gtk_widget_set_hexpand(g_entrada_comando, TRUE);
    GtkWidget *btn_ejecutar = gtk_button_new_with_label("Ejecutar");
    gtk_box_pack_start(GTK_BOX(barra_entrada), g_entrada_comando, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(barra_entrada), btn_ejecutar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), barra_entrada, FALSE, FALSE, 0);

    GtkWidget *barra_acciones = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *btn_historial = gtk_button_new_with_label("Ver historial");
    GtkWidget *btn_limpiar_hist = gtk_button_new_with_label("Limpiar historial");
    GtkWidget *btn_limpiar_pantalla = gtk_button_new_with_label("Limpiar pantalla");
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_historial, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_limpiar_hist, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_limpiar_pantalla, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), barra_acciones, FALSE, FALSE, 0);

    g_signal_connect(btn_ejecutar, "clicked", G_CALLBACK(on_ejecutar_clicked), NULL);
    g_signal_connect(g_entrada_comando, "activate", G_CALLBACK(on_ejecutar_clicked), NULL);
    g_signal_connect(btn_historial, "clicked", G_CALLBACK(on_historial_clicked), NULL);
    g_signal_connect(btn_limpiar_hist, "clicked", G_CALLBACK(on_limpiar_historial_clicked), NULL);
    g_signal_connect(btn_limpiar_pantalla, "clicked", G_CALLBACK(on_limpiar_pantalla_clicked), NULL);

    return caja;
}
