# === Paths ===
SRC_DIR     := src
BUILD_DIR   := build
BIN_DIR     := bin
INCLUDE_DIR := include

# === Compiler and flags ===
CC      := gcc
CFLAGS  := -Wvla -Wextra -Werror -I$(INCLUDE_DIR)

# === Programmi con un main ===
MAINS := direttore erogatore sportello operatore utente logger

# === Sorgenti/oggetti "comuni" (senza main) ===
COMMON_SRC := $(SRC_DIR)/utils.c
COMMON_OBJ := $(COMMON_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# === Target di default: tutti gli eseguibili ===
all: $(BIN_DIR) $(BUILD_DIR) $(addprefix $(BIN_DIR)/,$(MAINS))

# === Link eseguibili: ciascuno linka il proprio .o + gli oggetti comuni ===
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

$(BIN_DIR)/logger: $(BUILD_DIR)/logger.o $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@

# === Regola generica per compilare i .c in .o ===
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# === Crea le cartelle se mancano ===
$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

# === Clean ===
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
