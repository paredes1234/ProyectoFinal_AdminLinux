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

static void on_listar_clicked(GtkButton *btn, gpointer data);

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

static char *normalizar_ruta(const char *entrada) {
    const char *ruta = (entrada && entrada[0] != '\0') ? entrada : g_get_home_dir();
    char resuelta[PATH_MAX];
    if (realpath(ruta, resuelta)) return g_strdup(resuelta);
    return g_canonicalize_filename(ruta, NULL);
}

static void mostrar_directorio(const char *ruta_solicitada) {
    /* Invalida una búsqueda anterior para que sus resultados no reemplacen
       la carpeta a la que el usuario acaba de navegar. */
    g_generacion_busqueda++;
    if (g_spinner) gtk_spinner_stop(GTK_SPINNER(g_spinner));
    if (g_btn_buscar) gtk_widget_set_sensitive(g_btn_buscar, TRUE);

    char *ruta = normalizar_ruta(ruta_solicitada);
    ListaArchivos lista = archivos_obtener_lista(ruta);

    if (lista.items == NULL) {
        char *mensaje = g_strdup_printf("No se pudo abrir el directorio: %s", ruta);
        gui_mostrar_error(g_ventana, mensaje);
        g_free(mensaje);
        g_free(ruta);
        return;
    }

    g_modo_busqueda = FALSE;
    gtk_entry_set_text(GTK_ENTRY(g_entrada_ruta), ruta);
    refrescar_desde_lista(&lista);
    configurar_monitor_directorio(ruta);
    archivos_lista_liberar(&lista);
    g_free(ruta);
}

static void on_fila_activada(GtkTreeView *tree_view,
                             GtkTreePath *path,
                             GtkTreeViewColumn *column,
                             gpointer data) {
    (void) tree_view;
    (void) path;
    (void) column;
    (void) data;

    if (!seleccion_es_directorio()) return;
    char *ruta = obtener_ruta_seleccionada();
    if (ruta) {
        mostrar_directorio(ruta);
        g_free(ruta);
    }
}

static void on_ir_inicio_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;
    mostrar_directorio(g_get_home_dir());
}

static void on_ir_raiz_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;
    mostrar_directorio("/");
}

static void on_subir_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;
    const char *actual = gtk_entry_get_text(GTK_ENTRY(g_entrada_ruta));
    char *normalizada = normalizar_ruta(actual);
    char *padre = g_path_get_dirname(normalizada);
    mostrar_directorio(padre);
    g_free(padre);
    g_free(normalizada);
}

static char *obtener_ruta_seleccionada(void) {
    GtkTreeSelection *seleccion = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_treeview));
    GtkTreeModel *modelo;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(seleccion, &modelo, &iter)) return NULL;

    char *ruta = NULL;
    gtk_tree_model_get(modelo, &iter, COL_A_RUTA_COMPLETA, &ruta, -1);
    return ruta;
}

static gboolean seleccion_es_directorio(void) {
    GtkTreeSelection *seleccion = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_treeview));
    GtkTreeModel *modelo;
    GtkTreeIter iter;
    gboolean es_directorio = FALSE;
    if (gtk_tree_selection_get_selected(seleccion, &modelo, &iter)) {
        gtk_tree_model_get(modelo, &iter, COL_A_ES_DIRECTORIO, &es_directorio, -1);
    }
    return es_directorio;
}














