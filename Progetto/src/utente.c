#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>

int send_ticket_request() {
    key_t key = get_queue_key();
    int msg_id = init_msg_queue(key);

    request_msg req;
    req.mtype = MTYPE_REQUEST;
    req.service_type = 2;  //da rendere casuale******
    req.pid = getpid();

    if (msgsnd(msg_id, &req, sizeof(req) - sizeof(long), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }

    reply_msg reply;
    if (msgrcv(msg_id, &reply, sizeof(reply) - sizeof(long), req.pid, 0) == -1) {
        perror("msgrcv failed");
        exit(EXIT_FAILURE);
    }

    printf("[UTENTE %d] Received ticket number: %d\n", req.pid, reply.ticket_number);
    return 0;
}

int main(int argc, char *argv[]) {
    send_ticket_request();
}