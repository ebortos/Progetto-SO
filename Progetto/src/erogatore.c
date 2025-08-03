#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>


void issue_ticket(int sem_id, int msg_id) {
    int ticket_counter = 1;

    while (1) {     
        /* 1. attesa inizio giornata (bloccante) */
        sem_wait(sem_id, 0);
        printf("[EROGATORE] Inizio giornata lavorativa\n");

        /* 1.b  SUBITO dopo: fine simulazione?  */
        int v = semctl(sem_id, 2, GETVAL);

        if (v == -1) { 
            perror("semctl GETVAL"); 
            exit(EXIT_FAILURE);
        }

        if (v > 0) {
            printf("[EROGATORE] Fine simulazione rilevata (dopo wait).\n");
            return;
        }

        /* 2.  ciclo richieste finché direttore alza sem 1                  */
        while (1) {
            erogatore_request_msg req;
            ssize_t r = msgrcv(msg_id, &req, sizeof(req) - sizeof(long), MTYPE_REQUEST, IPC_NOWAIT);

            if (r != -1) {                       // richiesta presente
                printf("[EROGATORE] Ticket %d per servizio %d (PID %d)\n", ticket_counter, req.service_type, req.pid);

                erogatore_reply_msg rep = {
                    .mtype         = req.pid,
                    .ticket_number = ticket_counter++
                };

                if (msgsnd(msg_id, &rep, sizeof(rep) - sizeof(long), 0) == -1) {
                    perror("msgsnd"); 
                    exit(EXIT_FAILURE);
                }

            } else if (errno != ENOMSG) {       /* errore vero */
                perror("msgrcv"); 
                exit(EXIT_FAILURE);
            }

            /* 2.b – fine simulazione durante la giornata? */
            int t = sem_trywait(sem_id, 2);     /* tua helper: 1\|0\|-1 */

            if (t == 1) {
                printf("[EROGATORE] Fine simulazione durante la giornata.\n");
                struct sembuf rel = {2, 1, 0};
                semop(sem_id, &rel, 1);
                return;
            } else if (t == -1) {
                perror("sem_trywait sem2"); 
                exit(EXIT_FAILURE);
            }

            /* 2.c – fine giornata? */
            int e = sem_trywait(sem_id, 1);
            if (e == 1) {                       /* giorno terminato */
                struct sembuf rel = {1, 1, 0};
                semop(sem_id, &rel, 1);
                printf("[EROGATORE] Fine giornata, pausa...\n");
                break;
            } else if (e == -1) {
                perror("sem_trywait sem1"); 
                exit(EXIT_FAILURE);
            }

            /* 2.d – niente da fare: attesa passiva */
            if (r == -1) usleep(50000);
        }
    }
}



int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);   /* stdout line-buffered */

    key_t mq_key = get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    int msg_id = init_msg_queue(mq_key);

    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 3, 0);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    sem_signal(sem_id, 3);  //ready
    printf("[EROGATORE] Inizializzato e pronto a lavorare (PID %d)\n", getpid());
    
    issue_ticket(sem_id, msg_id);
    printf("[EROGATORE] Terminazione completata.\n");

    return 0;
}
