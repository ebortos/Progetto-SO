#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>


void issue_ticket(int sem_id, int msg_id, int log_qid) {
    int ticket_counter = 1;

    while (1) {     
        /* 1. attesa inizio giornata (bloccante) */
        sv_sem_wait(sem_id, 0);

        /* 1.b  SUBITO dopo: fine simulazione?  */
        int v = semctl(sem_id, 2, GETVAL);

        if (v == -1) { 
            perror("semctl GETVAL"); 
            exit(EXIT_FAILURE);
        }

        if (v > 0) return; //fine sim

        /* 2.  ciclo richieste finché direttore alza sem 1                  */
        while (1) {
            erogatore_request_msg req;
            ssize_t r = msgrcv(msg_id, &req, sizeof(req) - sizeof(long), MTYPE_REQUEST, IPC_NOWAIT);

            if (r != -1) {                       // richiesta presente
                log_sendf(log_qid, "[EROGATORE] Ticket %d per servizio %d (PID %d)\n", ticket_counter, req.service_type, req.pid);

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
            int t = sv_sem_trywait(sem_id, 2);

            if (t == 1) return;

            else if (t == -1) {
                perror("sem_trywait sem2"); 
                exit(EXIT_FAILURE);
            }

            /* 2.c – fine giornata? */
            int e = sv_sem_trywait(sem_id, 1);
            if (e == 1) break;      /* giorno terminato */    
            else if (e == -1) {
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

    int log_qid = open_log_queue();

    key_t mq_key = get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    int msg_id = init_msg_queue(mq_key);

    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 3, 0);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    sv_sem_signal(sem_id, 3);  //ready
    log_sendf(log_qid, "[EROGATORE] Inizializzato e pronto a lavorare (PID %d)\n", getpid());
    
    issue_ticket(sem_id, msg_id, log_qid);

    return 0;
}
