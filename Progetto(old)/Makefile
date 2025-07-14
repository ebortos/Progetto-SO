CC = gcc
CFLAGS = -Wvla -Wextra -Werror -Wall -O2
LDFLAGS = -pthread

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Sorgenti
DIRETTORE_SPORTELLO_SRC = $(SRC_DIR)/direttore.c $(SRC_DIR)/sportello.c
UTENTE_SRC = $(SRC_DIR)/utente.c
OPERATORE_SRC = $(SRC_DIR)/operatore.c
EROGATORE_SRC = $(SRC_DIR)/erogatore.c
IPC_UTILS_SRC = $(SRC_DIR)/ipc_utils.c

# Oggetti
DIRETTORE_SPORTELLO_OBJ = $(BUILD_DIR)/direttore.o $(BUILD_DIR)/sportello.o $(BUILD_DIR)/ipc_utils.o
UTENTE_OBJ = $(BUILD_DIR)/utente.o $(BUILD_DIR)/ipc_utils.o
OPERATORE_OBJ = $(BUILD_DIR)/operatore.o $(BUILD_DIR)/sportello.o $(BUILD_DIR)/ipc_utils.o
EROGATORE_OBJ = $(BUILD_DIR)/erogatore.o $(BUILD_DIR)/ipc_utils.o

# Eseguibili
DIRETTORE_SPORTELLO_EXE = $(BIN_DIR)/simulazione
UTENTE_EXE = $(BIN_DIR)/utente
OPERATORE_EXE = $(BIN_DIR)/operatore
EROGATORE_EXE = $(BIN_DIR)/erogatore

all: folders $(DIRETTORE_SPORTELLO_EXE) $(UTENTE_EXE) $(OPERATORE_EXE) $(EROGATORE_EXE)

# Regola generica per compilare i file oggetto
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Collegamento dei binari
$(DIRETTORE_SPORTELLO_EXE): $(DIRETTORE_SPORTELLO_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(UTENTE_EXE): $(UTENTE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OPERATORE_EXE): $(OPERATORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(EROGATORE_EXE): $(EROGATORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Creazione delle cartelle necessarie
folders:
	mkdir -p $(BUILD_DIR) $(BIN_DIR)

# Pulizia dei file generati
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
