#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>


//riceve servizio da direttore
static int try_receive_assignment(int spor_qid, int spor_id, sportello_t *out) {
    sportello_msg_t m;
    ssize_t r = msgrcv(spor_qid, &m, MSGSZ(sportello_msg_t), (1000 + spor_id), IPC_NOWAIT);
    if (r == -1) {
        if (errno == ENOMSG) return 0;
        perror("msgrcv assignment (sportello)");
        exit(EXIT_FAILURE);
    }
    if (out) *out = m.sportello_info;
    return 1;
}

void run_sportello(int spor_id, int sem_id, int log_qid, int spor_qid) {
    while (1) {
        //wait start of day
        sv_sem_wait(sem_id, 0);

        //end of sim right after waking?
        int v = semctl(sem_id, 2, GETVAL);
        if (v == -1) { perror("semctl GETVAL"); exit(EXIT_FAILURE); }
        if (v > 0) return;

        //new day
        int assigned = 0;
        sportello_t my = {0};

        while (1) {
            int did = 0;

            if (!assigned) {
                if (try_receive_assignment(spor_qid, spor_id, &my) == 1) {
                    assigned = 1;
                    
                    //if (log_qid >= 0) log_sendf(log_qid, "[SPORTELLO %d] assegnato servizio %d (op=%d)\n", spor_id, my.service_type, my.operatore_id);
                did = 1;
                }
            }

            //end of sim (during day)
            int t = sv_sem_trywait(sem_id, 2);
            if (t == 1) return;
            if (t == -1) { perror("sv_sem_trywait sem2 (sportello)"); exit(EXIT_FAILURE); }

            //end of day?
            int e = sv_sem_trywait(sem_id, 1);

            if (e == 1) {
                sv_sem_signal(sem_id, 3); //end of day!
                break;
            }
            
            if (e == -1) { perror("sv_sem_trywait sem1 (sportello)"); exit(EXIT_FAILURE); }

            if (!did) sched_yield();
        }
        //next day
    }
}



int main(int argc, char *argv[]) {
    //Init logger
    int sportello_id = atoi(argv[1]);

    key_t log_key = get_queue_key(FTOK_PATH_LOG, MSG_QUEUE_ID_LOG);
    int log_qid = init_msg_queue(log_key);

    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 4, 0);
    if (sem_id == -1) { perror("semget"); exit(EXIT_FAILURE); }

    //assignment queue: direttore -> sportelli
    int spor_qid = init_msg_queue(get_queue_key(FTOK_PATH_SPOR, MSG_QUEUE_ID_SPOR));

    //ready
    sv_sem_signal(sem_id, 3);

    run_sportello(sportello_id, sem_id, log_qid, spor_qid);
    return 0;
}