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

int remove_msg_queue(key_t key) {
    int msg_id = msgget(key, 0666);  // Access the queue (don't recreate)
    if (msg_id == -1) {
        perror("Error accessing message queue");
        return -1;
    }

    if (msgctl(msg_id, IPC_RMID, NULL) == -1) {
        perror("Error removing message queue");
        return -1;
    }

    printf("Message queue removed.\n");
    return 0;
}