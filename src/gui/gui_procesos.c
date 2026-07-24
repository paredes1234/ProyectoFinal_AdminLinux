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

GtkWidget *gui_procesos_crear_tab(GtkWidget *ventana_principal) {
    g_ventana = ventana_principal;

    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(caja), 10);

    /* Barra superior: búsqueda */
    GtkWidget *barra_busqueda = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    g_entrada_busqueda = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_entrada_busqueda), "Buscar proceso por nombre...");
    gtk_widget_set_hexpand(g_entrada_busqueda, TRUE);
    GtkWidget *btn_buscar = gtk_button_new_with_label("Buscar");
    GtkWidget *btn_actualizar = gtk_button_new_with_label("Actualizar");
    gtk_box_pack_start(GTK_BOX(barra_busqueda), g_entrada_busqueda, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(barra_busqueda), btn_buscar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_busqueda), btn_actualizar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), barra_busqueda, FALSE, FALSE, 0);

    /* Tabla de procesos */
    g_store = gtk_list_store_new(N_COLUMNAS_PROC, G_TYPE_INT, G_TYPE_INT,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_LONG);
    g_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_store));

    const char *titulos[] = { "PID", "PPID", "Nombre", "Estado", "Mem (KB)" };
    for (int i = 0; i < N_COLUMNAS_PROC; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *columna = gtk_tree_view_column_new_with_attributes(
            titulos[i], renderer, "text", i, NULL);
        gtk_tree_view_column_set_resizable(columna, TRUE);
        gtk_tree_view_column_set_sort_column_id(columna, i);
        gtk_tree_view_append_column(GTK_TREE_VIEW(g_treeview), columna);
    }
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(g_store), COL_MEM, GTK_SORT_DESCENDING);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(scroll), g_treeview);
    gtk_box_pack_start(GTK_BOX(caja), scroll, TRUE, TRUE, 0);

    /* Barra inferior: acciones */
    GtkWidget *barra_acciones = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *btn_cpu_mem = gtk_button_new_with_label("Ver CPU / Memoria");
    GtkWidget *btn_finalizar = gtk_button_new_with_label("Finalizar");
    GtkWidget *btn_suspender = gtk_button_new_with_label("Suspender");
    GtkWidget *btn_reanudar = gtk_button_new_with_label("Reanudar");
    GtkWidget *btn_arbol = gtk_button_new_with_label("Ver árbol de procesos");
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_cpu_mem, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_finalizar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_suspender, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_reanudar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barra_acciones), btn_arbol, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caja), barra_acciones, FALSE, FALSE, 0);

    g_etiqueta_estado = gtk_label_new("0 proceso(s)");
    gtk_widget_set_halign(g_etiqueta_estado, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(caja), g_etiqueta_estado, FALSE, FALSE, 0);

    g_signal_connect(btn_actualizar, "clicked", G_CALLBACK(on_actualizar_clicked), NULL);
    g_signal_connect(btn_buscar, "clicked", G_CALLBACK(on_buscar_clicked), NULL);
    g_signal_connect(g_entrada_busqueda, "activate", G_CALLBACK(on_buscar_clicked), NULL);
    g_signal_connect(btn_cpu_mem, "clicked", G_CALLBACK(on_cpu_mem_clicked), NULL);
    g_signal_connect(btn_finalizar, "clicked", G_CALLBACK(on_finalizar_clicked), NULL);
    g_signal_connect(btn_suspender, "clicked", G_CALLBACK(on_suspender_clicked), NULL);
    g_signal_connect(btn_reanudar, "clicked", G_CALLBACK(on_reanudar_clicked), NULL);
    g_signal_connect(btn_arbol, "clicked", G_CALLBACK(on_arbol_clicked), NULL);

    /* Carga inicial */
    on_actualizar_clicked(NULL, NULL);

    return caja;
}
