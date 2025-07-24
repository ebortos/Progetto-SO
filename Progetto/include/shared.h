#ifndef SHARED_H
#define SHARED_H

#define _GNU_SOURCE //per pid_t (mi sembra)(anche perchè è richiesto nei requisiti)

struct config_t;
struct dati_sim_t;

//tipologie servizi (da verificare se è utlizzato da più file, altrimenti recluderlo nell'unico bastardo che lo usa)
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

int init_msg_queue(key_t key);
int remove_msg_queue(key_t key);

#endif