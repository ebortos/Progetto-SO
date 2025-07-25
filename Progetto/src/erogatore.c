#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>

int issue_ticket() {
    key_t key = get_queue_key();
    int msg_id = init_msg_queue(key);

    int ticket_counter = 1;
    
    while (1) {
        request_msg req;
        if (msgrcv(msg_id, &req, sizeof(req) - sizeof(long), MTYPE_REQUEST, 0) == -1) {
            perror("msgrcv (request) failed");
            exit(EXIT_FAILURE);
        }

        printf("[EROGATORE] Received request for service %d from PID %d\n", req.service_type, req.pid);

        reply_msg reply;
        reply.mtype = req.pid;
        reply.ticket_number = ticket_counter++;

        if (msgsnd(msg_id, &reply, sizeof(reply) - sizeof(long), 0) == -1) {
            perror("msgsnd (reply) failed");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    issue_ticket();
}
