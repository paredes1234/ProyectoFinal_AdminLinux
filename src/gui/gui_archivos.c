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

static gboolean ejecutar_refresco_programado(gpointer data) {
    (void) data;
    g_refresco_pendiente = 0;
    if (!g_modo_busqueda) on_listar_clicked(NULL, NULL);
    return G_SOURCE_REMOVE;
}

static void on_cambio_directorio(GFileMonitor *monitor,
                                 GFile *archivo,
                                 GFile *otro_archivo,
                                 GFileMonitorEvent evento,
                                 gpointer data) {
    (void) monitor;
    (void) archivo;
    (void) otro_archivo;
    (void) evento;
    (void) data;

    if (g_modo_busqueda) return;
    if (g_refresco_pendiente != 0) g_source_remove(g_refresco_pendiente);
    g_refresco_pendiente = g_timeout_add(250, ejecutar_refresco_programado, NULL);
}

static void configurar_monitor_directorio(const char *ruta) {
    if (g_monitor_directorio) {
        g_file_monitor_cancel(g_monitor_directorio);
        g_clear_object(&g_monitor_directorio);
    }

    GFile *directorio = g_file_new_for_path(ruta);
    GError *error = NULL;
    g_monitor_directorio = g_file_monitor_directory(
        directorio, G_FILE_MONITOR_WATCH_MOVES, NULL, &error);
    g_object_unref(directorio);

    if (!g_monitor_directorio) {
        char *mensaje = g_strdup_printf("Ruta actual: %s (sin monitoreo automático: %s)",
                                        ruta, error ? error->message : "error desconocido");
        establecer_estado(mensaje);
        g_free(mensaje);
        g_clear_error(&error);
        return;
    }

    g_signal_connect(g_monitor_directorio, "changed",
                     G_CALLBACK(on_cambio_directorio), NULL);

    char *mensaje = g_strdup_printf(
        "Monitoreando automáticamente: %s — los cambios aparecen sin pulsar Listar", ruta);
    establecer_estado(mensaje);
    g_free(mensaje);
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

static void on_listar_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;
    mostrar_directorio(gtk_entry_get_text(GTK_ENTRY(g_entrada_ruta)));
}

/* Devuelve la ruta completa almacenada en la fila seleccionada. */
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

static void on_copiar_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;
    char *origen = obtener_ruta_seleccionada();
    if (!origen) {
        gui_mostrar_error(g_ventana, "Selecciona un archivo o carpeta de la lista.");
        return;
    }

    gboolean es_directorio = seleccion_es_directorio();
    char *sugerencia = g_strdup_printf("%s_copia", origen);
    char *destino = gui_pedir_texto(
        g_ventana,
        es_directorio ? "Copiar carpeta completa" : "Copiar archivo",
        "Ruta nueva o carpeta de destino:",
        sugerencia);
    g_free(sugerencia);

    if (destino) {
        g_strstrip(destino);
        if (destino[0] == '\0') {
            gui_mostrar_error(g_ventana, "La ruta de destino no puede estar vacía.");
        } else if (archivos_copiar(origen, destino) == 0) {
            gui_mostrar_info(g_ventana, "Copiar", "Elemento copiado correctamente.");
            if (!g_modo_busqueda) on_listar_clicked(NULL, NULL);
        } else {
            int error_copia = errno;
            char *mensaje = g_strdup_printf(
                "No se pudo copiar el elemento.\n\nDetalle: %s",
                g_strerror(error_copia));
            gui_mostrar_error(g_ventana, mensaje);
            g_free(mensaje);
        }
        g_free(destino);
    }
    g_free(origen);
}

static void on_mover_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;
    char *origen = obtener_ruta_seleccionada();
    if (!origen) {
        gui_mostrar_error(g_ventana, "Selecciona un archivo o carpeta de la lista.");
        return;
    }

    char *destino = gui_pedir_texto(g_ventana, "Mover elemento", "Ruta de destino:", origen);
    if (destino) {
        if (archivos_mover(origen, destino) == 0) {
            gui_mostrar_info(g_ventana, "Mover", "Elemento movido correctamente.");
            if (!g_modo_busqueda) on_listar_clicked(NULL, NULL);
        } else {
            gui_mostrar_error(g_ventana, "No se pudo mover el elemento.");
        }
        g_free(destino);
    }
    g_free(origen);
}

static void on_eliminar_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;
    char *ruta = obtener_ruta_seleccionada();
    if (!ruta) {
        gui_mostrar_error(g_ventana, "Selecciona un elemento de la lista.");
        return;
    }

    GtkWidget *confirmacion = gtk_message_dialog_new(
        GTK_WINDOW(g_ventana), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO, "¿Eliminar \"%s\"?", ruta);
    int respuesta = gtk_dialog_run(GTK_DIALOG(confirmacion));
    gtk_widget_destroy(confirmacion);

    if (respuesta == GTK_RESPONSE_YES) {
        if (archivos_eliminar(ruta) == 0) {
            if (!g_modo_busqueda) on_listar_clicked(NULL, NULL);
            else gtk_list_store_clear(g_store);
        } else {
            gui_mostrar_error(g_ventana,
                "No se pudo eliminar. Las carpetas deben estar vacías y debes tener permisos.");
        }
    }
    g_free(ruta);
}

typedef struct {
    char *base;
    char *patron;
    guint generacion;
} TareaBusqueda;

typedef struct {
    ListaArchivos lista;
    char *base;
    char *patron;
    guint generacion;
} ResultadoBusquedaGui;

static gboolean mostrar_resultado_busqueda(gpointer data) {
    ResultadoBusquedaGui *resultado = data;

    if (resultado->generacion == g_generacion_busqueda) {
        gtk_spinner_stop(GTK_SPINNER(g_spinner));
        gtk_widget_set_sensitive(g_btn_buscar, TRUE);
        g_modo_busqueda = TRUE;
        refrescar_desde_lista(&resultado->lista);

        char *mensaje;
        if (resultado->lista.truncada) {
            mensaje = g_strdup_printf(
                "%d resultados para \"%s\" desde %s (lista limitada para evitar bloquear el sistema)",
                resultado->lista.total, resultado->patron, resultado->base);
        } else {
            mensaje = g_strdup_printf("%d resultado(s) para \"%s\" desde %s",
                                      resultado->lista.total, resultado->patron, resultado->base);
        }
        establecer_estado(mensaje);
        g_free(mensaje);

        if (resultado->lista.total == 0) {
            gui_mostrar_info(g_ventana, "Búsqueda", "No se encontraron coincidencias.");
        }
    }

    archivos_lista_liberar(&resultado->lista);
    g_free(resultado->base);
    g_free(resultado->patron);
    g_free(resultado);
    return G_SOURCE_REMOVE;
}

static gpointer ejecutar_busqueda_hilo(gpointer data) {
    TareaBusqueda *tarea = data;
    ResultadoBusquedaGui *resultado = g_new0(ResultadoBusquedaGui, 1);
    resultado->base = g_strdup(tarea->base);
    resultado->patron = g_strdup(tarea->patron);
    resultado->generacion = tarea->generacion;
    resultado->lista = archivos_buscar_lista(tarea->base, tarea->patron);

    g_idle_add(mostrar_resultado_busqueda, resultado);
    g_free(tarea->base);
    g_free(tarea->patron);
    g_free(tarea);
    return NULL;
}

static void on_buscar_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;

    char *patron = gui_pedir_texto(
        g_ventana, "Buscar archivos y carpetas", "Texto contenido en el nombre:", "");
    if (!patron) return;
    g_strstrip(patron);
    if (patron[0] == '\0') {
        g_free(patron);
        return;
    }

    gboolean global = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_check_busqueda_global));
    char *base = global
        ? g_strdup("/")
        : normalizar_ruta(gtk_entry_get_text(GTK_ENTRY(g_entrada_ruta)));

    g_generacion_busqueda++;
    gtk_widget_set_sensitive(g_btn_buscar, FALSE);
    gtk_spinner_start(GTK_SPINNER(g_spinner));

    char *mensaje = g_strdup_printf("Buscando \"%s\" desde %s...", patron, base);
    establecer_estado(mensaje);
    g_free(mensaje);

    TareaBusqueda *tarea = g_new0(TareaBusqueda, 1);
    tarea->base = base;
    tarea->patron = patron;
    tarea->generacion = g_generacion_busqueda;

    GThread *hilo = g_thread_new("busqueda-archivos", ejecutar_busqueda_hilo, tarea);
    g_thread_unref(hilo);
}

static void on_crear_carpeta_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;
    if (g_modo_busqueda) {
        gui_mostrar_error(g_ventana, "Primero entra a una carpeta usando Inicio, Raíz o Listar.");
        return;
    }

    char *nombre = gui_pedir_texto(g_ventana, "Nueva carpeta", "Nombre de la carpeta:", "Nueva carpeta");
    if (!nombre) return;
    g_strstrip(nombre);
    if (nombre[0] == '\0' || strchr(nombre, '/')) {
        gui_mostrar_error(g_ventana, "Escribe un nombre válido, sin '/'.");
        g_free(nombre);
        return;
    }

    const char *base = gtk_entry_get_text(GTK_ENTRY(g_entrada_ruta));
    char *ruta = g_build_filename(base, nombre, NULL);
    if (archivos_crear_directorio(ruta) != 0)
        gui_mostrar_error(g_ventana, "No se pudo crear la carpeta. Revisa permisos o si ya existe.");
    g_free(ruta);
    g_free(nombre);
}

static void on_crear_archivo_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;
    if (g_modo_busqueda) {
        gui_mostrar_error(g_ventana, "Primero entra a una carpeta usando Inicio, Raíz o Listar.");
        return;
    }

    char *nombre = gui_pedir_texto(g_ventana, "Nuevo archivo", "Nombre del archivo:", "nuevo.txt");
    if (!nombre) return;
    g_strstrip(nombre);
    if (nombre[0] == '\0' || strchr(nombre, '/')) {
        gui_mostrar_error(g_ventana, "Escribe un nombre válido, sin '/'.");
        g_free(nombre);
        return;
    }

    const char *base = gtk_entry_get_text(GTK_ENTRY(g_entrada_ruta));
    char *ruta = g_build_filename(base, nombre, NULL);
    if (archivos_crear_archivo(ruta) != 0)
        gui_mostrar_error(g_ventana, "No se pudo crear el archivo. Revisa permisos o si ya existe.");
    g_free(ruta);
    g_free(nombre);
}

static void on_estadisticas_clicked(GtkButton *btn, gpointer data) {
    (void) btn;
    (void) data;
    char *ruta = obtener_ruta_seleccionada();
    if (!ruta) {
        gui_mostrar_error(g_ventana, "Selecciona un elemento de la lista.");
        return;
    }

    char *texto = archivos_estadisticas_texto(ruta);
    gui_mostrar_info(g_ventana, "Estadísticas", texto);
    free(texto);
    g_free(ruta);
}

GtkWidget *gui_archivos_crear_tab(GtkWidget *ventana_principal) {
    g_ventana = ventana_principal;

    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(caja), 10);

    GtkWidget *barra_navegacion = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *btn_inicio = gtk_button_new_with_label("Inicio");
    GtkWidget *btn_raiz = gtk_button_new_with_label("Raíz /");
    GtkWidget *btn_subir = gtk_button_new_with_label("Subir");
    g_entrada_ruta = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(g_entrada_ruta), g_get_home_dir());
    gtk_widget_set_hexpand(g_entrada_ruta, TRUE);
    GtkWidget *btn_listar = gtk_button_new_with_label("Listar / actualizar");

    gtk_box_pack_start(GTK_BOX(barra_navegacion), btn_inicio, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_navegacion), btn_raiz, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_navegacion), btn_subir, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_navegacion), g_entrada_ruta, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(barra_navegacion), btn_listar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), barra_navegacion, FALSE, FALSE, 0);

    GtkWidget *barra_busqueda = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    g_btn_buscar = gtk_button_new_with_label("Buscar...");
    g_check_busqueda_global = gtk_check_button_new_with_label("Buscar en todo el sistema");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_check_busqueda_global), TRUE);
    g_spinner = gtk_spinner_new();
    g_label_estado = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(g_label_estado), 0.0f);
    gtk_widget_set_hexpand(g_label_estado, TRUE);

    gtk_box_pack_start(GTK_BOX(barra_busqueda), g_btn_buscar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_busqueda), g_check_busqueda_global, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_busqueda), g_spinner, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_busqueda), g_label_estado, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(caja), barra_busqueda, FALSE, FALSE, 0);

    g_store = gtk_list_store_new(
        N_COLUMNAS_ARCH,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_BOOLEAN);
    g_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_store));
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(g_treeview), TRUE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(g_treeview), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(g_treeview), COL_A_NOMBRE);

    const char *titulos[] = {"Nombre", "Tipo", "Tamaño", "Ubicación"};
    for (int i = 0; i < 4; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *columna = gtk_tree_view_column_new_with_attributes(
            titulos[i], renderer, "text", i, NULL);
        gtk_tree_view_column_set_resizable(columna, TRUE);
        gtk_tree_view_column_set_expand(columna, i == COL_A_NOMBRE || i == COL_A_UBICACION);
        gtk_tree_view_column_set_sort_column_id(columna, i);
        gtk_tree_view_append_column(GTK_TREE_VIEW(g_treeview), columna);
    }

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(scroll), g_treeview);
    gtk_box_pack_start(GTK_BOX(caja), scroll, TRUE, TRUE, 0);

    GtkWidget *barra_acciones = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *btn_nueva_carpeta = gtk_button_new_with_label("Nueva carpeta");
    GtkWidget *btn_nuevo_archivo = gtk_button_new_with_label("Nuevo archivo");
    GtkWidget *btn_copiar = gtk_button_new_with_label("Copiar");
    GtkWidget *btn_mover = gtk_button_new_with_label("Mover");
    GtkWidget *btn_eliminar = gtk_button_new_with_label("Eliminar");
    GtkWidget *btn_stats = gtk_button_new_with_label("Estadísticas");

    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_nueva_carpeta, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_nuevo_archivo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_copiar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_mover, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_eliminar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_stats, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), barra_acciones, FALSE, FALSE, 0);

    g_signal_connect(btn_inicio, "clicked", G_CALLBACK(on_ir_inicio_clicked), NULL);
    g_signal_connect(btn_raiz, "clicked", G_CALLBACK(on_ir_raiz_clicked), NULL);
    g_signal_connect(btn_subir, "clicked", G_CALLBACK(on_subir_clicked), NULL);
    g_signal_connect(btn_listar, "clicked", G_CALLBACK(on_listar_clicked), NULL);
    g_signal_connect(g_entrada_ruta, "activate", G_CALLBACK(on_listar_clicked), NULL);
    g_signal_connect(g_btn_buscar, "clicked", G_CALLBACK(on_buscar_clicked), NULL);
    g_signal_connect(g_treeview, "row-activated", G_CALLBACK(on_fila_activada), NULL);
    g_signal_connect(btn_nueva_carpeta, "clicked", G_CALLBACK(on_crear_carpeta_clicked), NULL);
    g_signal_connect(btn_nuevo_archivo, "clicked", G_CALLBACK(on_crear_archivo_clicked), NULL);
    g_signal_connect(btn_copiar, "clicked", G_CALLBACK(on_copiar_clicked), NULL);
    g_signal_connect(btn_mover, "clicked", G_CALLBACK(on_mover_clicked), NULL);
    g_signal_connect(btn_eliminar, "clicked", G_CALLBACK(on_eliminar_clicked), NULL);
    g_signal_connect(btn_stats, "clicked", G_CALLBACK(on_estadisticas_clicked), NULL);

    mostrar_directorio(g_get_home_dir());
    return caja;
}
