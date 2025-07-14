//AGGIUNGERE DESTROY MSG QUEUE

#include "../include/ipc_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int create_semaphore(key_t key, int nsems) {
    int sem_id = semget(key, nsems, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    return sem_id;
}

void ipc_sem_wait(int sem_id, int sem_num) {
    struct sembuf sb = {sem_num, -1, 0};
    if (semop(sem_id, &sb, 1) == -1) {
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
}

void ipc_sem_signal(int sem_id, int sem_num) {
    struct sembuf sb = {sem_num, 1, 0};
    if (semop(sem_id, &sb, 1) == -1) {
        perror("sem_signal");
        exit(EXIT_FAILURE);
    }
}

void remove_semaphore(int sem_id) {
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
    }
}

int create_shared_memory(key_t key, size_t size) {
    int shm_id = shmget(key, size, IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shm_id;
}

void *attach_shared_memory(int shm_id) {
    void *shm_addr = shmat(shm_id, NULL, 0);
    if (shm_addr == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return shm_addr;
}

void detach_shared_memory(void *shm_addr) {
    if (shmdt(shm_addr) == -1) {
        perror("shmdt");
    }
}

void remove_shared_memory(int shm_id) {
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
    }
}

int init_message_queue(key_t key) {
    int msg_id = msgget(key, IPC_CREAT | 0666);
    if (msg_id == -1) {
        perror("Error creating meassge queue");
        exit(EXIT_FAILURE);
    }
    return msg_id;
}

void destroy_message_queue(int msg_id) {
    if (msgctl(msg_id, IPC_RMID, NULL) == -1) {
        perror("msgctl IPC_RMID");
    }
}
