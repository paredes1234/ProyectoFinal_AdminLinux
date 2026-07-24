#ifndef GUI_COMMON_H
#define GUI_COMMON_H

#include <gtk/gtk.h>

/* Muestra un diálogo modal simple de información o error */
void gui_mostrar_info(GtkWidget *padre, const char *titulo, const char *mensaje);
void gui_mostrar_error(GtkWidget *padre, const char *mensaje);

/* Pide al usuario un valor de texto mediante un diálogo con un GtkEntry.
   Devuelve una cadena reservada con g_strdup (el llamador debe hacer g_free)
   o NULL si el usuario canceló. */
char *gui_pedir_texto(GtkWidget *padre, const char *titulo, const char *etiqueta, const char *valor_inicial);

/* Muestra un bloque de texto largo (por ejemplo salida de comandos o reportes)
   dentro de un diálogo con scroll. */
void gui_mostrar_texto_largo(GtkWidget *padre, const char *titulo, const char *texto);

/* Construye cada pestaña principal de la aplicación */
GtkWidget *gui_procesos_crear_tab(GtkWidget *ventana_principal);
GtkWidget *gui_archivos_crear_tab(GtkWidget *ventana_principal);
GtkWidget *gui_comandos_crear_tab(GtkWidget *ventana_principal);
GtkWidget *gui_respaldos_crear_tab(GtkWidget *ventana_principal);

#endif
