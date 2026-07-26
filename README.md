# ADMIN — Herramienta de Administración Linux

![Lenguaje C](https://img.shields.io/badge/Lenguaje-C%20GNU11-00599C?logo=c&logoColor=white)
![Interfaz GTK3](https://img.shields.io/badge/Interfaz-GTK3-4A86CF?logo=gtk&logoColor=white)
![Plataforma Linux](https://img.shields.io/badge/Plataforma-Linux-FCC624?logo=linux&logoColor=black)

**ADMIN** es una aplicación desarrollada en C para administrar tareas frecuentes de un sistema Linux mediante una interfaz de terminal y una interfaz gráfica construida con GTK3.

El programa integra en una sola herramienta la gestión de procesos, operaciones con archivos y carpetas, ejecución de comandos, respaldos incrementales, análisis básico de scripts Bash y una cola demostrativa de descargas.

## Características principales

| Módulo | Funciones |
|---|---|
| **Procesos** | Listar procesos activos, buscar por nombre, consultar PID, PPID, estado, CPU y memoria, finalizar, suspender, reanudar y visualizar el árbol de procesos. |
| **Archivos** | Navegar por directorios, listar, crear, copiar, mover y eliminar archivos o carpetas, realizar búsquedas recursivas y consultar estadísticas. |
| **Comandos** | Ejecutar comandos Linux, capturar la salida estándar y de error, consultar el código de salida y mantener un historial. |
| **Respaldos** | Crear respaldos completos e incrementales, listar versiones disponibles y restaurar una versión en otra ubicación. |
| **Analizador Bash** | Examinar scripts `.sh` e identificar variables, ciclos `for`, `while`, `until`, condicionales y funciones. |
| **Cola de descargas** | Agregar elementos a una cola y representar su procesamiento mediante una barra de progreso. |

> [!IMPORTANT]
> La cola de descargas es una demostración del manejo de colas y callbacks de progreso. La versión actual **simula el avance** y no realiza una descarga HTTP real.

## Interfaces disponibles

El proyecto genera dos ejecutables:

- `admin`: interfaz de línea de comandos.
- `admin-gui`: interfaz gráfica GTK3.

Ambas interfaces utilizan los mismos módulos internos, por lo que comparten la lógica principal del sistema.

## Requisitos

- Sistema operativo Linux.
- GCC con soporte para GNU11.
- GNU Make.
- `pkg-config`.
- GTK3 para compilar la interfaz gráfica.

En Ubuntu, Linux Mint o distribuciones derivadas de Debian:

```bash
sudo apt update
sudo apt install -y build-essential pkg-config libgtk-3-dev
```

## Instalación

### 1. Clonar el repositorio

```bash
git clone https://github.com/paredes1234/ProyectoFinal_AdminLinux.git
cd ProyectoFinal_AdminLinux
```

### 2. Compilar el proyecto completo

```bash
make
```

Este comando genera:

```text
admin
admin-gui
```

### 3. Ejecutar la interfaz gráfica

```bash
./admin-gui
```

También puedes compilar y ejecutar directamente con:

```bash
make run-gui
```

### 4. Ejecutar la interfaz de terminal

```bash
./admin
```

También puedes utilizar:

```bash
make run
```

## Objetivos del Makefile

| Comando | Descripción |
|---|---|
| `make` | Compila la interfaz CLI y la interfaz gráfica. |
| `make cli` | Genera únicamente el ejecutable `admin`. |
| `make gui` | Genera únicamente el ejecutable `admin-gui`. |
| `make run` | Compila y ejecuta la versión CLI. |
| `make run-gui` | Compila y ejecuta la versión gráfica. |
| `make clean` | Elimina objetos, ejecutables y archivos de registro generados. |

Para recompilar completamente:

```bash
make clean
make
```

## Uso de la interfaz gráfica

Al iniciar `admin-gui` se muestra una ventana con cuatro pestañas.

### Procesos

1. Presiona **Actualizar** para volver a cargar los procesos.
2. Escribe un nombre para filtrar la lista.
3. Selecciona una fila para consultar CPU y memoria.
4. Usa las acciones **Finalizar**, **Suspender** o **Reanudar** cuando sea necesario.
5. Presiona **Árbol de procesos** para visualizar la relación padre-hijo.

### Archivos

1. Usa **Inicio**, **Raíz /** y **Subir** para navegar por el sistema.
2. Haz doble clic en una carpeta para abrirla.
3. Selecciona un elemento para copiarlo, moverlo, eliminarlo o consultar sus estadísticas.
4. Utiliza **Nueva carpeta** o **Nuevo archivo** para crear elementos.
5. Activa la búsqueda global para buscar desde `/`.

La vista se actualiza cuando se producen cambios en el directorio observado. La búsqueda global evita directorios virtuales como `/proc`, `/sys`, `/dev` y `/run`.

### Comandos

1. Escribe un comando Linux.
2. Presiona **Ejecutar**.
3. Revisa la salida estándar, los errores y el código de salida.
4. Consulta o limpia el historial desde los botones de la pestaña.

Ejemplo seguro:

```bash
uname -a
```

### Respaldos, Bash y descargas

Esta pestaña agrupa tres herramientas:

- **Respaldos:** selecciona una carpeta de origen y una carpeta de destino, crea una versión, lista las versiones disponibles y restaura la que necesites.
- **Analizador Bash:** selecciona un archivo `.sh` para obtener un resumen de variables y estructuras de control.
- **Cola:** agrega rutas o direcciones de ejemplo y procesa la cola para observar el avance simulado.

## Uso desde terminal

El ejecutable `admin` presenta el siguiente menú:

```text
=== ADMIN - Herramienta de Administración Linux ===
1. Administrador de Tareas
2. Shell de Archivos
3. Comandos Linux
4. Respaldos / Análisis Bash / Descargas
0. Salir
```

Cada opción abre un submenú con las operaciones del módulo correspondiente. La opción `0` regresa al menú anterior o cierra el programa desde el menú principal.

## Estructura del proyecto

```text
ProyectoFinal_AdminLinux/
├── include/
│   ├── archivos.h
│   ├── bash_analyzer.h
│   ├── comandos.h
│   ├── gui_common.h
│   ├── procesos.h
│   ├── respaldos.h
│   └── utils.h
├── src/
│   ├── gui/
│   │   ├── gui_archivos.c
│   │   ├── gui_comandos.c
│   │   ├── gui_common.c
│   │   ├── gui_procesos.c
│   │   └── gui_respaldos.c
│   ├── archivos.c
│   ├── bash_analyzer.c
│   ├── comandos.c
│   ├── main.c
│   ├── main_gui.c
│   ├── procesos.c
│   ├── respaldos.c
│   └── utils.c
├── docs/
│   ├── Documentacion_Tecnica_ADMIN_Final.pdf
│   └── Manual_de_Usuario_ADMIN_Final.pdf
├── Makefile
└── README.md
```

## Archivos y carpetas generados

Durante la compilación y ejecución se pueden crear:

```text
obj/                  Objetos generados por el compilador.
logs/                 Registros del programa e historial de comandos.
respaldos_data/       Carpeta preparada para almacenar respaldos.
admin                 Ejecutable CLI.
admin-gui             Ejecutable GTK3.
```

Estos elementos se pueden eliminar con:

```bash
make clean
```

## Permisos y seguridad

- El programa debe ejecutarse normalmente con los permisos del usuario actual.
- Las operaciones sobre procesos de otros usuarios o directorios protegidos pueden devolver `Permission denied`.
- Finalizar o suspender procesos esenciales puede afectar la sesión o la estabilidad del sistema.
- La ejecución de comandos permite utilizar el shell del sistema; revisa cada comando antes de ejecutarlo.
- Se recomienda probar las funciones de eliminación, movimiento y restauración con archivos de prueba.
- Evita ejecutar toda la aplicación como superusuario salvo que sea estrictamente necesario.

## Limitaciones conocidas

- El proyecto depende de funciones de Linux como `/proc` y señales POSIX, por lo que no se ejecuta de forma nativa en Windows.
- La interfaz gráfica requiere GTK3.
- La cola de descargas simula el progreso y no descarga datos de Internet.
- Los respaldos incrementales seleccionan archivos según su fecha de modificación.
- La disponibilidad de algunas operaciones depende de los permisos del usuario.

## Solución de problemas

### `make: command not found`

```bash
sudo apt install build-essential
```

### `Package gtk+-3.0 was not found`

```bash
sudo apt install pkg-config libgtk-3-dev
```

### `make: No targets specified and no makefile found`

Verifica que te encuentres en la raíz del repositorio:

```bash
pwd
ls
```

En la salida debe aparecer el archivo `Makefile`.

### `./admin-gui: Permission denied`

```bash
chmod +x admin-gui
./admin-gui
```

### No se puede modificar un proceso o archivo

Comprueba que el recurso pertenezca al usuario actual y que tengas permisos suficientes. No se recomienda usar `sudo` como primera solución.

## Documentación

- [Manual de usuario](docs/Manual_de_Usuario_ADMIN_Final.pdf)
- [Documentación técnica](docs/Documentacion_Tecnica_ADMIN_Final.pdf)

## Tecnologías utilizadas

- C GNU11.
- GTK3 y GLib.
- GNU Make.
- API POSIX.
- Sistema de archivos de Linux.
- `/proc` para obtener información de procesos.
- Git y GitHub para control de versiones.

## Contexto académico

Proyecto desarrollado como trabajo final del curso de **Programación de Sistemas** de la carrera de **Ingeniería de Sistemas**.

---

<p align="center">
  <strong>ADMIN — Administración de Linux desde CLI y GTK3</strong>
</p>
