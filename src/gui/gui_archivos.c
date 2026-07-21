#include "../../include/gui_common.h"
#include "../../include/archivos.h"
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

enum {
    COL_A_NOMBRE,
    COL_A_TIPO,
    COL_A_TAMANO,
    COL_A_UBICACION,
    COL_A_RUTA_COMPLETA,
    COL_A_ES_DIRECTORIO,
    N_COLUMNAS_ARCH
};

static GtkWidget *g_ventana;
static GtkListStore *g_store;
static GtkWidget *g_treeview;
static GtkWidget *g_entrada_ruta;
static GtkWidget *g_check_busqueda_global;
static GtkWidget *g_label_estado;
static GtkWidget *g_btn_buscar;
static GtkWidget *g_spinner;

static GFileMonitor *g_monitor_directorio;
static guint g_refresco_pendiente;
static guint g_generacion_busqueda;
static gboolean g_modo_busqueda;

static void formatear_bytes_local(long long bytes, char *salida, int tam) {
    const char *unidades[] = {"B", "KB", "MB", "GB", "TB"};
    double valor = (double) bytes;
    int i = 0;
    while (valor >= 1024.0 && i < 4) {
        valor /= 1024.0;
        i++;
    }
    snprintf(salida, tam, "%.2f %s", valor, unidades[i]);
}

static void establecer_estado(const char *texto) {
    gtk_label_set_text(GTK_LABEL(g_label_estado), texto ? texto : "");
}

static void refrescar_desde_lista(const ListaArchivos *lista) {
    gtk_list_store_clear(g_store);
    GtkTreeIter iter;
    char tam_fmt[32];

    for (int i = 0; i < lista->total; i++) {
        const ItemArchivo *it = &lista->items[i];
        formatear_bytes_local(it->tamano, tam_fmt, sizeof(tam_fmt));

        char *ubicacion = g_path_get_dirname(it->ruta);
        gtk_list_store_append(g_store, &iter);
        gtk_list_store_set(g_store, &iter,
            COL_A_NOMBRE, it->nombre,
            COL_A_TIPO, it->es_directorio ? "CARPETA" : "ARCHIVO",
            COL_A_TAMANO, tam_fmt,
            COL_A_UBICACION, ubicacion,
            COL_A_RUTA_COMPLETA, it->ruta,
            COL_A_ES_DIRECTORIO, it->es_directorio,
            -1);
        g_free(ubicacion);
    }
}


