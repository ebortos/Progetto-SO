#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

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