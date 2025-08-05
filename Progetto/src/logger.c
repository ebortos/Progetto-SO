#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);  /* flush per riga */

    /* semafori (0 start,1 stop,2 end,3 ready) */
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

    /* coda logger */
    int log_qid = open_log_queue();

    /* barriera ready */
    sem_signal(sem_id, 3);

    /* loop ricezione */
    while (1) {
        log_msg_t m;
        ssize_t r = msgrcv(log_qid, &m, sizeof(m) - sizeof(long), 0, 0);
        
        if (r == -1) { 
            perror("msgrcv logger"); 
            exit(EXIT_FAILURE); 
        }

        if (m.mtype == MTYPE_LOG_SHUTDOWN) {
            printf("[LOGGER] shutdown\n");
            break;
        }

        /* stampa la riga così com’è (già formattata dai mittenti) */
        fputs(m.text, stdout);
        fflush(stdout);
    }

    //cleanup ipc
    msgctl(log_qid, IPC_RMID, NULL);

    return 0;
}