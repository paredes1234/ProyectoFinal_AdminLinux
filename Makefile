CC := gcc
CFLAGS := -Wall -Wextra -std=gnu11 -g
SRC_DIR := src
OBJ_DIR := obj
BIN_CLI := admin
BIN_GUI := admin-gui

# Módulos núcleo compartidos por el CLI y la GUI
CORE_SRCS := $(SRC_DIR)/procesos.c $(SRC_DIR)/archivos.c $(SRC_DIR)/comandos.c \
             $(SRC_DIR)/respaldos.c $(SRC_DIR)/bash_analyzer.c $(SRC_DIR)/utils.c
CORE_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CORE_SRCS))

GUI_SRCS := $(SRC_DIR)/main_gui.c $(wildcard $(SRC_DIR)/gui/*.c)
GUI_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(GUI_SRCS))

GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS := $(shell pkg-config --libs gtk+-3.0 2>/dev/null)

.PHONY: all cli gui clean run run-gui dirs

all: cli gui

cli: dirs $(BIN_CLI)

gui: dirs $(BIN_GUI)

dirs:
	@mkdir -p $(OBJ_DIR)/gui logs respaldos_data

# --- CLI ---
$(BIN_CLI): $(CORE_OBJS) $(OBJ_DIR)/main.o
	$(CC) $(CFLAGS) -o $@ $^

# --- GUI (requiere GTK3: sudo apt install libgtk-3-dev) ---
$(BIN_GUI): $(CORE_OBJS) $(GUI_OBJS)
ifeq ($(strip $(GTK_CFLAGS)),)
	@echo "ERROR: no se encontró GTK3. Instala con: sudo apt install libgtk-3-dev pkg-config"
	@exit 1
endif
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $^ $(GTK_LIBS)

$(OBJ_DIR)/gui/%.o: $(SRC_DIR)/gui/%.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(OBJ_DIR)/main_gui.o: $(SRC_DIR)/main_gui.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: cli
	./$(BIN_CLI)

run-gui: gui
	./$(BIN_GUI)

clean:
	rm -rf $(OBJ_DIR) $(BIN_CLI) $(BIN_GUI) logs/*.log
