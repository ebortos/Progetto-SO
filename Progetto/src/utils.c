#define _GNU_SOURCE

#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>
#include <errno.h>

//Msg Queue
int init_msg_queue(key_t key) {
    int msg_id = msgget(key, IPC_CREAT | 0666);
    
    if (msg_id == -1) {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }

    return msg_id;
}

key_t get_queue_key(const char *path, char id) {
    key_t key = ftok(path, id);

    if (key == -1) {
        perror("ftok failed");
        exit(EXIT_FAILURE);
    }

    return key;
}

void remove_msg_queue(int msg_id) {
    if (msgctl(msg_id, IPC_RMID, NULL) == -1) {
        perror("msgctl (remove) failed");
        exit(EXIT_FAILURE);
    }
}

//Signal handlers
volatile sig_atomic_t day_start = 0;
volatile sig_atomic_t day_end = 0;

static void handle_day_start(int sig) {
    day_start = 1;
}

static void handle_day_end(int sig) {
    day_end = 1;
}

static void handle_termination(int sig) {
    printf("[PID %d] Ricevuto SIGTERM, terminazione.\n", getpid());
    _exit(EXIT_SUCCESS);
}

void setup_signal_handlers(void) {
    struct sigaction sa = {0};

    sa.sa_handler = handle_day_start;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_day_end;
    sigaction(SIGUSR2, &sa, NULL);

    sa.sa_handler = handle_termination;
    sigaction(SIGTERM, &sa, NULL);
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

void remove_semaphore_set(int sem_id) {
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl - IPC_RMID");
    }
}

void sem_wait(int sem_id, int sem_num) {
    struct sembuf op = {sem_num, -1, 0};

    if (semop(sem_id, &op, 1) == -1) {
        perror("semop - wait");
        exit(EXIT_FAILURE);
    }
}

void sem_signal(int sem_id, int sem_num) {
    struct sembuf op = {sem_num, 1, 0};

    if (semop(sem_id, &op, 1) == -1) {
        perror("semop - signal");
        exit(EXIT_FAILURE);
    }
}

void sem_set(int sem_id, int sem_num, int value) {
    union semun arg;
    arg.val = value;
    if (semctl(sem_id, sem_num, SETVAL, arg) == -1) {
        perror("semctl SETVAL failed");
        exit(EXIT_FAILURE);
    }
}