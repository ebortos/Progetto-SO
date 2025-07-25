#ifndef SHARED_H
#define SHARED_H

#include <sys/ipc.h>
#include <unistd.h>

#define _GNU_SOURCE //per pid_t (mi sembra)(anche perchè è richiesto nei requisiti)

#define FTOK_PATH "../config.conf" 
#define MSG_QUEUE_ID 'M'
#define MTYPE_REQUEST 1 
#define MTYPE_REPLY 2

//tipologie servizi (da verificare se è utlizzato da più file, altrimenti recluderlo nell'unico bastardo che lo usa)
typedef enum {
    PACCHI = 0,
    LETTERE = 1,
    BANCOPOSTA = 2,
    BOLLETTINI = 3,
    PRODOTTI_FINANZIARI = 4,
    OROLOGI = 5
} service_type_t;

//sportello
typedef struct {
    int service_type;     //servizio erogato
    int occupato;          //0 = libero, 1 = occupato
    int operatore_id;      //operatore assegnato
} sportello_t;

typedef struct {
    long mtype;
    int service_type;
    pid_t pid;              //pid del richiedente
} request_msg;

typedef struct {
    long mtype;
    int ticket_number;
} reply_msg;

int init_msg_queue(key_t key);
key_t get_queue_key();
void remove_msg_queue(key_t key);

#endif