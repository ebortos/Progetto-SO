#ifndef SHARED_H
#define SHARED_H

#include <sys/ipc.h>
#include <unistd.h>

#define FTOK_PATH_EROG "../tmp/ipc_msg_key"
#define MSG_QUEUE_ID_EROG 'A'
#define MTYPE_REQUEST 1 
#define MTYPE_REPLY 2

#define FTOK_PATH_SPOR "../tmp/ipc_msg_key2"
#define MSG_QUEUE_ID_SPOR 'B'

#define FTOK_PATH_SEM "../tmp/ipc_sem_key"
#define SEM_KEY_ID 'C'

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

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int init_msg_queue(key_t key);
key_t get_queue_key(const char *path, char id);
void remove_msg_queue(key_t key);

void setup_signal_handlers(void);
void sigterm_handler(int signum);

int create_semaphore_set(key_t key, int nsems);
void remove_semaphore_set(int sem_id);
void sem_wait(int sem_id, int sem_num);
int sem_trywait(int sem_id, int sem_num);
void sem_signal(int sem_id, int sem_num);
void sem_set(int sem_id, int sem_num, int value);

void sem_debug(const char *tag, int sem_id, int nsems);

#endif