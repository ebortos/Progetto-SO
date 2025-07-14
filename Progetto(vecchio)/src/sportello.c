#include "../include/ipc_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>

#define _GNU_SOURCE
#define MAX_SPORTELLI 10

typedef struct {
    int id;
    int servizio;
    int is_occupied;
    int sem_id;
} Sportello;

static Sportello sportelli[MAX_SPORTELLI];
static int num_sportelli;

void inizializza_sportelli(int n, int sem_id_base) {
    if (n > MAX_SPORTELLI) {
        perror("Error: troppi sportelli\n");
        exit(EXIT_FAILURE);
    }

    num_sportelli = n;
    
    for (int i = 0; i < num_sportelli; i++) {
        sportelli[i].id = i;
        sportelli[i].servizio = -1;
        sportelli[i].is_occupied = 0;
        sportelli[i].sem_id = sem_id_base + i;
    }
    
    printf("Aperti %d sportelli.\n", num_sportelli);
}

int cerca_sportello_libero(Sportello *sportelli, int num_sportelli, int servizio, int semaforo_globale) {
    ipc_sem_wait(semaforo_globale, servizio);

    int sportello_id = -1;
    for (int i = 0; i < num_sportelli; i++) {
        ipc_sem_wait(sportelli[i].sem_id, 0);

        if (!sportelli[i].is_occupied && sportelli[i].servizio == servizio) {
            sportelli[i].is_occupied = 1;
            sportello_id = sportelli[i].id;
        }

        ipc_sem_signal(sportelli[i].sem_id, 0);

        if (sportello_id != -1) 
            break;
    }

    ipc_sem_signal(semaforo_globale, servizio);
    return sportello_id;
}



void libera_sportello(int sportello_id) {
    if (sportello_id < 0 || sportello_id >= num_sportelli) {
        fprintf(stderr, "Error: ID sportello non valido (%d).\n", sportello_id);
        exit(EXIT_FAILURE);
    }
    
    ipc_sem_wait(sportelli[sportello_id].sem_id, 0);
    sportelli[sportello_id].is_occupied = 0;
    ipc_sem_signal(sportelli[sportello_id].sem_id, 0);
    
    printf("Sportello %d liberato.\n", sportello_id);
}

void distruggi_sportelli() {
    for (int i = 0; i < num_sportelli; i++) {
        if (semctl(sportelli[i].sem_id, 0, IPC_RMID) == -1) {
            fprintf(stderr, "Errore durante la rimozione del semaforo %d (sportello %d).\n", sportelli[i].sem_id, i);
        } else {
            printf("Sportello %d distrutto correttamente.\n", i);
        }
    }
    printf("Tutti gli sportelli sono stati distrutti.\n");
}
