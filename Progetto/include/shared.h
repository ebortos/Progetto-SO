#ifndef SHARED_H
#define SHARED_H

#include <sys/ipc.h>
#include <unistd.h>

#define _GNU_SOURCE //per pid_t (mi sembra)(anche perchè è richiesto nei requisiti)

#define FTOK_PATH_EROG "../config.conf" 
#define MSG_QUEUE_ID_EROG 'A'
#define MTYPE_REQUEST 1 
#define MTYPE_REPLY 2

#define FTOK_PATH_SPOR "../Makefile"
#define MSG_QUEUE_ID_SPOR 'B'

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
    sportello_t sportello_info;
} sportello_msg_t;

typedef struct {
    long mtype;
    int service_type;
    pid_t pid;              //pid del richiedente
} erogatore_request_msg;

typedef struct {
    long mtype;
    int ticket_number;
} erogatore_reply_msg;

int init_msg_queue(key_t key);
key_t get_queue_key(const char *path, char id);
void remove_msg_queue(key_t key);

#endif