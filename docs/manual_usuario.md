# Manual de Usuario — ADMIN

## Requisitos

- Linux (probado en Ubuntu 24.04).
- `gcc`, `make` y `pkg-config` instalados.
- Para la interfaz gráfica: `libgtk-3-dev`.

```bash
sudo apt install build-essential pkg-config libgtk-3-dev
```

## Instalación y ejecución

```bash
make            # compila el CLI (admin) y la GUI (admin-gui)
./admin-gui     # abre la interfaz gráfica
```

Si prefieres la interfaz de texto (por ejemplo, para usar por SSH sin entorno
gráfico), ejecuta `./admin` en su lugar. Ambas comparten la misma lógica y
producen los mismos resultados.

## Interfaz gráfica

Al abrir `admin-gui` verás una ventana con cuatro pestañas, una por cada
módulo. La navegación es la misma en todas: escribes los datos en los campos
de texto de la parte superior, presionas el botón de acción, y el resultado
aparece en la tabla, el área de texto o un cuadro de diálogo.

### Pestaña Procesos

Al abrir la pestaña se carga automáticamente la lista de procesos. Puedes:

- **Buscar** escribiendo un nombre y presionando "Buscar" (o Enter).
- Hacer clic sobre una fila para seleccionar un proceso, y luego usar los
  botones **Ver CPU / Memoria**, **Finalizar**, **Suspender**, **Reanudar**.
- **Ver árbol de procesos** abre una ventana con la jerarquía completa.

> Nota: finalizar o suspender procesos que no te pertenecen requiere permisos
> de superusuario (ejecuta `sudo ./admin-gui`).

### Pestaña Archivos

La pestaña abre inicialmente la carpeta personal del usuario. Usa **Inicio**,
**Raíz /** y **Subir** para recorrer todo el sistema. También puedes escribir
una ruta y presionar **Listar / actualizar**. Haz doble clic sobre una carpeta
para entrar en ella.

La tabla se actualiza automáticamente cuando se crea, elimina, mueve o cambia
de nombre un archivo o una carpeta dentro del directorio mostrado. También
puedes usar **Nueva carpeta** y **Nuevo archivo** para crear elementos desde la
propia aplicación.

Selecciona una fila para **Copiar**, **Mover**, **Eliminar** o consultar sus
**Estadísticas**. La columna **Ubicación** muestra la ruta donde se encuentra.
El botón **Buscar...** busca por nombre; con **Buscar en todo el sistema**
activado, la búsqueda comienza en `/`, y desactivado busca solamente desde la
ruta actual. La búsqueda global se realiza en segundo plano y omite `/proc`,
`/sys`, `/dev` y `/run` para evitar recorrer sistemas virtuales.

### Pestaña Comandos

Escribe un comando y presiona **Ejecutar** (o Enter). La salida estándar se
muestra en verde, los errores en rojo, y cada ejecución queda registrada
en el historial (**Ver historial**). **Limpiar pantalla** solo borra la vista,
no el historial guardado en disco.

### Pestaña Respaldos / Bash / Descargas

Tres secciones independientes:

- **Respaldos incrementales**: indica origen y carpeta de respaldos, presiona
  **Crear respaldo**. Usa **Listar versiones** para poblar el selector y
  **Restaurar** para copiar una versión a la ruta indicada.
- **Análisis de scripts Bash**: indica la ruta de un `.sh` y presiona
  **Analizar**; el reporte (variables, ciclos, funciones) aparece debajo.
- **Cola de descargas**: agrega elementos con **Agregar**, y **Procesar cola**
  para ver el avance en la barra de progreso.

## Uso por línea de comandos (`./admin`)

Al iniciar verás el menú principal:

```
=== ADMIN - Herramienta de Administración Linux ===
1. Administrador de Tareas
2. Shell de Archivos
3. Comandos Linux
4. Respaldos / Análisis Bash / Descargas
0. Salir
```

## 1. Administrador de Tareas

Permite listar procesos activos, buscar por nombre, ver su consumo de CPU/memoria,
finalizarlos, suspenderlos/reanudarlos y visualizar el árbol de procesos.

> Nota: finalizar o suspender procesos que no te pertenecen requiere permisos
> de superusuario (`sudo ./admin`).

## 2. Shell de Archivos

Listar, copiar, mover, eliminar y buscar archivos, además de ver estadísticas
(tamaño, permisos, fecha de modificación) de cualquier ruta.

Ejemplo — buscar todos los archivos `.log` dentro de un proyecto:

```
Opción: 5
Directorio base: /home/usuario/proyecto
Patrón a buscar: .log
```

## 3. Comandos Linux

Ejecuta cualquier comando de shell y muestra su salida estándar y de error
por separado, junto al código de salida. Cada comando queda registrado
en `logs/historial_comandos.log`.

## 4. Respaldos, Análisis Bash y Descargas

- **Respaldo incremental**: indica una carpeta origen y una carpeta de
  respaldos. La primera vez copia todo; las siguientes veces solo copia
  los archivos modificados desde el último respaldo.
- **Restaurar versión**: elige la carpeta de versión (nombre con timestamp,
  visible con la opción "Listar versiones") y la ruta donde restaurar.
- **Analizar script Bash**: indica la ruta de un `.sh` y obtén un reporte de
  variables, ciclos (`for`/`while`/`until`), condicionales y funciones.
- **Cola de descargas**: agrega rutas/URLs a una cola y procésala para ver
  el progreso de cada elemento simulado con una barra de avance.

## Estructura de carpetas generadas

```
logs/                       -> admin.log, historial_comandos.log
<carpeta de respaldos>/     -> una subcarpeta por versión (timestamp)
                                y el archivo oculto .ultimo_respaldo
```

## Salir

Opción `0` en cualquier submenú regresa al menú anterior; `0` en el menú
principal cierra el programa.


### Copia de archivos y carpetas
La interfaz permite copiar archivos individuales y carpetas completas de forma recursiva. Al copiar una carpeta se conservan sus subcarpetas y archivos. Por seguridad, no se permite copiar una carpeta dentro de sí misma ni sobrescribir una carpeta de destino que ya exista.
