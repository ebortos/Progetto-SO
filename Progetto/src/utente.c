#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>

int send_ticket_request() {
    key_t key = get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    int msg_id = init_msg_queue(key);

    srand(time(NULL) ^ getpid());

    erogatore_request_msg req;
    req.mtype = MTYPE_REQUEST;
    req.service_type = rand() % 6;
    req.pid = getpid();

    if (msgsnd(msg_id, &req, sizeof(req) - sizeof(long), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }

    erogatore_reply_msg reply;
    if (msgrcv(msg_id, &reply, sizeof(reply) - sizeof(long), req.pid, 0) == -1) {
        perror("msgrcv failed");
        exit(EXIT_FAILURE);
    }

    printf("[UTENTE %d] Ricevuto ticket numero: %d\n", req.pid, reply.ticket_number);

    //remove_msg_queue(msg_id); 
    return 0;
}

int main(int argc, char *argv[]) {
    send_ticket_request();

    return 0;
}