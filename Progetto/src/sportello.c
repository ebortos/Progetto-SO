#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>


int receive_service(int spor_id) {
    key_t key = get_queue_key(FTOK_PATH_SPOR, MSG_QUEUE_ID_SPOR);
    int msg_id = init_msg_queue(key);

    while (1) {
        sportello_msg_t msg;

        if (msgrcv(msg_id, &msg, sizeof(sportello_t), 1000 + spor_id, 0) == -1) {
            perror("Errore ricezione messaggio");
            exit(EXIT_FAILURE);
        }

        sportello_t stato = msg.sportello_info;
        printf("[Sportello %d] Servizio assegnato: %d | Operatore: %d\n", spor_id, stato.service_type, stato.operatore_id);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int spor_id = atoi(argv[1]);
    receive_service(spor_id);

    return 0;
}