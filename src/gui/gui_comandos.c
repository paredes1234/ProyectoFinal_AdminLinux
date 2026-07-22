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



