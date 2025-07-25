# MAKEFILE TEMPORANEO (palese da chatgpt), ricontrollare alla fine
# per semplificare la compilazione

# === Paths ===
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
INCLUDE_DIR := include

# === Compiler and flags ===
CC := gcc
CFLAGS := -Wvla -Wextra -Werror -I$(INCLUDE_DIR)

# === Source files ===
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))
COMMON_OBJ := $(BUILD_DIR)/ipc_utils.o

# === Executables (one per .c file with main) ===
EXECUTABLES := direttore erogatore sportello operatore utente# add other main files here manually

# === Default target ===
all: $(BIN_DIR) $(BUILD_DIR) $(addprefix $(BIN_DIR)/, $(EXECUTABLES))

# === Executable targets ===
$(BIN_DIR)/direttore: $(BUILD_DIR)/direttore.o $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@

$(BIN_DIR)/erogatore: $(BUILD_DIR)/erogatore.o $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@

$(BIN_DIR)/utente: $(BUILD_DIR)/utente.o $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@

$(BIN_DIR)/sportello: $(BUILD_DIR)/sportello.o $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@

$(BIN_DIR)/operatore: $(BUILD_DIR)/operatore.o $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@

# === Pattern rule for building .o files ===
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# === Create build and bin folders if not present ===
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# === Clean ===
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
