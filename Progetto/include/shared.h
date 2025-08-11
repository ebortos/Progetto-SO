#ifndef SHARED_H
#define SHARED_H

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <unistd.h>

#define FTOK_PATH_LOG "../tmp/ipc_msg_log_key"
#define MSG_QUEUE_ID_LOG 'L'
#define LOG_TEXT_MAX 256

#define FTOK_PATH_EROG "../tmp/ipc_msg_key"
#define MSG_QUEUE_ID_EROG 'A'
#define MTYPE_REQUEST 1 
#define MTYPE_REPLY 2

#define FTOK_PATH_SPOR "../tmp/ipc_msg_key2"
#define MSG_QUEUE_ID_SPOR 'B'

#define FTOK_PATH_SEM "../tmp/ipc_sem_key"
#define SEM_KEY_ID 'C'

#define FTOK_PATH_SERV   "../tmp/ftok_service"   /* queue: utente -> sportello */
#define FTOK_PATH_DONE   "../tmp/ftok_done"      /* queue: sportello -> utente */
#define MSG_QUEUE_ID_SERV  'S'
#define MSG_QUEUE_ID_DONE  'D'

#define NUM_SERVICES 6
#define FTOK_PATH_PLAN "../tmp/ftok_plan"
#define SHM_PLAN_ID    'P'

#define MSGSZ(T) ((int)(sizeof(T) - sizeof(long)))

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
    int service_type;      //servizio erogato
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
    int ticket_number;      //0 when asking erogatore; set when queueing for sportello
} erogatore_request_msg;

typedef struct {
    long mtype;
    int ticket_number;
    int service_type;
    //int  served_ok;      1=served, 0=failed (if you need) statistiche?
} erogatore_reply_msg;

typedef struct {
    int counts[NUM_SERVICES];   // how many sportelli serve each service today
    //int day;                     optional: current sim day | statistiche?
} day_plan_t;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

typedef struct {
    long mtype;
    char text[LOG_TEXT_MAX];
} log_msg_t;

enum {
    MTYPE_LOG_LINE     = 1,
    MTYPE_LOG_SHUTDOWN = 255
};

int init_msg_queue(key_t key);
int init_msg_queue_fresh(key_t key); 
key_t get_queue_key(const char *path, char id);
void remove_msg_queue(key_t key);

int create_semaphore_set(key_t key, int nsems);
void remove_semaphore_set(key_t key);
void sv_sem_wait(int sem_id, int sem_num);
int sv_sem_trywait(int sem_id, int sem_num);
void sv_sem_signal(int sem_id, int sem_num);
void sv_sem_set(int sem_id, int sem_num, int value);

int open_log_queue(void);
int log_sendf(int log_qid, const char *fmt, ...);
int log_send_shutdown(int log_qid);

void cleanup_all_ipc(void);
int purge_queue_all(int qid);

int open_service_queue(void);
int open_done_queue(void);

int shm_get_or_create(key_t key, size_t size);
int shm_get_existing(key_t key);
void* shm_attach(int shmid, int readonly);
void shm_detach(const void *addr);
void shm_remove(int shmid);

#endif