#include "../../include/gui_common.h"

void gui_mostrar_info(GtkWidget *padre, const char *titulo, const char *mensaje) {
    GtkWidget *dialogo = gtk_message_dialog_new(GTK_WINDOW(padre), GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", mensaje);
    gtk_window_set_title(GTK_WINDOW(dialogo), titulo);
    gtk_dialog_run(GTK_DIALOG(dialogo));
    gtk_widget_destroy(dialogo);
}

void gui_mostrar_error(GtkWidget *padre, const char *mensaje) {
    GtkWidget *dialogo = gtk_message_dialog_new(GTK_WINDOW(padre), GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", mensaje);
    gtk_window_set_title(GTK_WINDOW(dialogo), "Error");
    gtk_dialog_run(GTK_DIALOG(dialogo));
    gtk_widget_destroy(dialogo);
}

char *gui_pedir_texto(GtkWidget *padre, const char *titulo, const char *etiqueta, const char *valor_inicial) {
    GtkWidget *dialogo = gtk_dialog_new_with_buttons(titulo, GTK_WINDOW(padre), GTK_DIALOG_MODAL,
        "_Cancelar", GTK_RESPONSE_CANCEL, "_Aceptar", GTK_RESPONSE_OK, NULL);

    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialogo));
    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(caja), 10);

    GtkWidget *etiq = gtk_label_new(etiqueta);
    gtk_widget_set_halign(etiq, GTK_ALIGN_START);
    GtkWidget *entrada = gtk_entry_new();
    if (valor_inicial) gtk_entry_set_text(GTK_ENTRY(entrada), valor_inicial);
    gtk_entry_set_activates_default(GTK_ENTRY(entrada), TRUE);

    gtk_box_pack_start(GTK_BOX(caja), etiq, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), entrada, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(area), caja);
    gtk_widget_set_size_request(dialogo, 380, -1);

    gtk_dialog_set_default_response(GTK_DIALOG(dialogo), GTK_RESPONSE_OK);
    gtk_widget_show_all(dialogo);

    char *resultado = NULL;
    if (gtk_dialog_run(GTK_DIALOG(dialogo)) == GTK_RESPONSE_OK) {
        resultado = g_strdup(gtk_entry_get_text(GTK_ENTRY(entrada)));
    }
    gtk_widget_destroy(dialogo);
    return resultado;
}

void gui_mostrar_texto_largo(GtkWidget *padre, const char *titulo, const char *texto) {
    GtkWidget *dialogo = gtk_dialog_new_with_buttons(titulo, GTK_WINDOW(padre), GTK_DIALOG_MODAL,
        "_Cerrar", GTK_RESPONSE_CLOSE, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialogo), 640, 480);

    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialogo));
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *vista = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(vista), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(vista), TRUE);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(vista));
    gtk_text_buffer_set_text(buffer, texto, -1);

    gtk_container_add(GTK_CONTAINER(scroll), vista);
    gtk_box_pack_start(GTK_BOX(area), scroll, TRUE, TRUE, 0);
    gtk_widget_show_all(dialogo);

    gtk_dialog_run(GTK_DIALOG(dialogo));
    gtk_widget_destroy(dialogo);
}
