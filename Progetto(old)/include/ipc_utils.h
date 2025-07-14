#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h> 


int create_semaphore(key_t key, int nsems);
void ipc_sem_wait(int sem_id, int sem_num);
void ipc_sem_signal(int sem_id, int sem_num);
void remove_semaphore(int sem_id);

int create_shared_memory(key_t key, size_t size);
void *attach_shared_memory(int shm_id);
void detach_shared_memory(void *shm_addr);
void remove_shared_memory(int shm_id);

int init_message_queue(key_t key);
void destroy_message_queue(int msg_id);

#endif
