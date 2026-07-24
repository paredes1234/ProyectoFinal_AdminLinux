#ifndef UTILS_H
#define UTILS_H

#define COLOR_RESET   "\x1b[0m"
#define COLOR_TITULO  "\x1b[1;34m"
#define COLOR_OK      "\x1b[1;32m"
#define COLOR_ERROR   "\x1b[1;31m"
#define COLOR_INFO    "\x1b[1;33m"

void utils_titulo(const char *texto);
void utils_formatear_bytes(long bytes, char *salida, int tam);
void utils_timestamp(char *salida, int tam);
void utils_log(const char *modulo, const char *mensaje);

#endif
