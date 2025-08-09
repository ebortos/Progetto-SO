#define _GNU_SOURCE

#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdarg.h>

//Msg Queue
int init_msg_queue(key_t key) {
    int msg_id = msgget(key, IPC_CREAT | 0666);
    
    if (msg_id == -1) {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }

    return msg_id;
}

/* Remove the queue if it exists, then recreate it empty.
   Use this ONCE at startup (best: in the director, before forking). */
int init_msg_queue_fresh(key_t key) {
    /* Try create exclusively: if it works, it's brand new. */
    int id = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
    if (id != -1) return id;

    if (errno != EEXIST) {
        perror("msgget (fresh, create excl)");
        exit(EXIT_FAILURE);
    }

    /* It already exists: remove it. */
    int old = msgget(key, 0);
    if (old != -1) {
        if (msgctl(old, IPC_RMID, NULL) == -1) {
            perror("msgctl IPC_RMID (fresh)");
            exit(EXIT_FAILURE);
        }
    }

    /* Recreate. Tiny retry loop to be robust against races. */
    for (int i = 0; i < 4; ++i) {
        id = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
        if (id != -1) return id;

        if (errno != EEXIST) {
            perror("msgget (fresh, recreate)");
            exit(EXIT_FAILURE);
        }
        /* Another process recreated it between remove and create; remove again. */
        old = msgget(key, 0);
        if (old != -1) {
            msgctl(old, IPC_RMID, NULL);
        }
    }

    fprintf(stderr, "init_msg_queue_fresh: failed to recreate queue\n");
    exit(EXIT_FAILURE);
}


key_t get_queue_key(const char *path, char id) {
    key_t key = ftok(path, id);

    if (key == -1) {
        perror("ftok failed");
        exit(EXIT_FAILURE);
    }

    return key;
}

void remove_msg_queue(key_t key) {
    int id = msgget(key, 0);

    if (id == -1) {
        if (errno != ENOENT) 
            perror("msgget remove");
        return;
    }

    if (msgctl(id, IPC_RMID, NULL) == -1)
        perror("msgctl IPC_RMID");
}


//Semaphore (system V)
int create_semaphore_set(key_t key, int nsems) {
    int sem_id = semget(key, nsems, IPC_CREAT | 0666);

    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // inizializza tutti i semafori a 0
    union semun arg;
    unsigned short *init_vals = calloc(nsems, sizeof(unsigned short));

    if (!init_vals) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    arg.array = init_vals;

    if (semctl(sem_id, 0, SETALL, arg) == -1) {
        perror("semctl - SETALL");
        exit(EXIT_FAILURE);
    }

    free(init_vals);
    return sem_id;
}

void remove_semaphore_set(key_t key) {
    if (key == (key_t)-1) return;   //ftok fallita

    int sem_id = semget(key, 0, 0);
    if (sem_id == -1) {
        if (errno != ENOENT)
            perror("semget (remove by key)");
        return;
    }

    /* rimuovi il set; per IPC_RMID l'argomento è ignorato */
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        if (errno != EIDRM && errno != EINVAL)
            perror("semctl IPC_RMID (sem)");
    }
}

void sv_sem_wait(int sem_id, int sem_num) {
    struct sembuf op = { sem_num, -1, 0 };
    if (semop(sem_id, &op, 1) == -1) { 
        perror("semop - wait");
        exit(EXIT_FAILURE); 
    }
}

int sv_sem_trywait(int sem_id, int sem_num) {
    struct sembuf op = { sem_num, -1, IPC_NOWAIT };
    if (semop(sem_id, &op, 1) == -1) {
        return (errno == EAGAIN) ? 0 : -1;   // 0 = non disponibile, -1 = errore vero
    }
    
    return 1;                                // preso con successo
}

void sv_sem_signal(int sem_id, int sem_num) {
    struct sembuf op = {sem_num, 1, 0};

    if (semop(sem_id, &op, 1) == -1) {
        perror("semop - signal");
        exit(EXIT_FAILURE);
    }
}

void sv_sem_set(int sem_id, int sem_num, int value) {
    union semun arg;
    arg.val = value;
    if (semctl(sem_id, sem_num, SETVAL, arg) == -1) {
        perror("semctl SETVAL failed");
        exit(EXIT_FAILURE);
    }
}

//Logger
int open_log_queue() {
    key_t key = get_queue_key(FTOK_PATH_LOG, MSG_QUEUE_ID_LOG);
    return init_msg_queue(key);
}

int log_sendf(int log_qid, const char *fmt, ...) {
    log_msg_t m;
    m.mtype = MTYPE_LOG_LINE;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m.text, LOG_TEXT_MAX, fmt, ap);
    va_end(ap);

    return msgsnd(log_qid, &m, sizeof(m) - sizeof(long), 0);
}

/* Invia messaggio di shutdown al logger */
int log_send_shutdown(int log_qid) {
    log_msg_t m;
    m.mtype = MTYPE_LOG_SHUTDOWN;
    m.text[0] = '\0';
    return msgsnd(log_qid, &m, sizeof(m) - sizeof(long), 0);
}

//chiude tutte le ipc, aggiungere qua eventuali ipc nuove
void cleanup_all_ipc(void) {
    key_t k_sem  = ftok(FTOK_PATH_SEM,  SEM_KEY_ID);
    key_t k_log  = ftok(FTOK_PATH_LOG,  MSG_QUEUE_ID_LOG);
    key_t k_erog = ftok(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    key_t k_spor = ftok(FTOK_PATH_SPOR, MSG_QUEUE_ID_SPOR);

    remove_msg_queue(k_log);
    remove_msg_queue(k_erog);
    remove_msg_queue(k_spor);

    remove_semaphore_set(k_sem);
}

int open_service_queue(void) {
    key_t k = get_queue_key(FTOK_PATH_SERV, MSG_QUEUE_ID_SERV);
    return init_msg_queue(k);
}

int open_done_queue(void) {
    key_t k = get_queue_key(FTOK_PATH_DONE, MSG_QUEUE_ID_DONE);
    return init_msg_queue(k);
}

/* Remove every message currently in the queue (non-blocking). Returns count removed. */
int purge_queue_all(int qid) {
    int removed = 0;

    while (1) {
        char buf[256];
        ssize_t r = msgrcv(qid, buf, sizeof(buf), 0, IPC_NOWAIT);
        
        if (r == -1) {
            if (errno == ENOMSG) break;
            perror("msgrcv purge"); break;
        }
        removed++;
    }
    return removed;
}

//Shared memory
int shm_plan_get_existing(void) {
    key_t k = ftok(FTOK_PATH_PLAN, SHM_PLAN_ID);
    if (k == -1) { perror("ftok plan"); exit(EXIT_FAILURE); }
    int id = shmget(k, 0, 0);  // no IPC_CREAT here
    if (id == -1) { perror("shmget plan (get)"); exit(EXIT_FAILURE); }
    return id;
}

day_plan_t* shm_plan_attach_ro(int shmid) {
    void *p = shmat(shmid, NULL, SHM_RDONLY);
    if (p == (void *)-1) { perror("shmat plan ro"); exit(EXIT_FAILURE); }
    return (day_plan_t*)p;
}

void shm_plan_detach(const day_plan_t *p) {
    if (p) shmdt((const void*)p);
}
