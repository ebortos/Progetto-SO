#ifndef SPORTELLO_H
#define SPORTELLO_H

typedef struct {
    int id;
    int servizio;
    int is_occupied;
    int sem_id;
} Sportello;

void inizializza_sportelli(int n, int sem_id_base);
int cerca_sportello_libero(Sportello *sportelli, int num_sportelli, int servizio, int semaforo_globale);
void libera_sportello(int sportello_id);
void distruggi_sportelli();

#endif