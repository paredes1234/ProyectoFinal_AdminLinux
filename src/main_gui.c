#include <gtk/gtk.h>
#include <sys/stat.h>
#include "../include/gui_common.h"

static void activar(GtkApplication *app, gpointer user_data) {
    (void) user_data;

    GtkWidget *ventana = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(ventana), "ADMIN - Herramienta de Administración Linux");
    gtk_window_set_default_size(GTK_WINDOW(ventana), 900, 600);
    gtk_container_set_border_width(GTK_CONTAINER(ventana), 8);

    GtkWidget *notebook = gtk_notebook_new();

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        gui_procesos_crear_tab(ventana), gtk_label_new("  Procesos  "));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        gui_archivos_crear_tab(ventana), gtk_label_new("  Archivos  "));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        gui_comandos_crear_tab(ventana), gtk_label_new("  Comandos  "));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        gui_respaldos_crear_tab(ventana), gtk_label_new("  Respaldos / Bash / Descargas  "));

    gtk_container_add(GTK_CONTAINER(ventana), notebook);
    gtk_widget_show_all(ventana);
}

int main(int argc, char **argv) {
    mkdir("logs", 0755);

    GtkApplication *app = gtk_application_new("unsa.admin.gui", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activar), NULL);
    int estado = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return estado;
}
