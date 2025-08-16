#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    if (sem_key == -1) { 
        perror("ftok sem");
        exit(EXIT_FAILURE);
    }

    int sem_id = semget(sem_key, 4, 0);
    if (sem_id == -1) { 
        perror("semget"); 
        exit(EXIT_FAILURE); 
    }

    key_t log_key = get_queue_key(FTOK_PATH_LOG, MSG_QUEUE_ID_LOG);
    int log_qid = init_msg_queue(log_key);

    //ready
    sv_sem_signal(sem_id, 3);

    while (1) {
        log_msg_t m;
        ssize_t r = msgrcv(log_qid, &m, sizeof(m) - sizeof(long), 0, 0);
        
        if (r == -1) { 
            perror("msgrcv logger"); 
            exit(EXIT_FAILURE); 
        }

        if (m.mtype == MTYPE_LOG_SHUTDOWN) {
            log_sendf(log_qid, "[LOGGER] shutdown\n");
            break;
        }

        fputs(m.text, stdout);
        fflush(stdout);
    }

    return 0;
}