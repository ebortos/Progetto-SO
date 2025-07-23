#ifndef SHARED_H
#define SHARED_H

#define _GNU_SOURCE

struct config_t;
struct dati_sim_t;

//tipologie servizi
typedef enum {
    PACCHI = 0,
    LETTERE = 1,
    BANCOPOSTA = 2,
    BOLLETTINI = 3,
    PRODOTTI_FINANZIARI = 4,
    OROLOGI = 5
} tipo_servizio_t;

//sportello
typedef struct {
    int tipo_servizio;     //servizio erogato
    int occupato;          //0 = libero, 1 = occupato
    int operatore_id;      //operatore assegnato
} sportello_t;

#endif