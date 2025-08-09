#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>


void receive_service(int spor_id, int sem_id, int log_qid, int msg_id) {
    while (1) {
        /* 1) wait start-of-day */
        sv_sem_wait(sem_id, 0);

        /* end-of-sim immediately after waking? (don’t consume sem2) */
        int v = semctl(sem_id, 2, GETVAL);

        if (v == -1) { 
            perror("semctl GETVAL"); 
            exit(EXIT_FAILURE); 
        }
        if (v > 0) return;

        /* new day: expect one assignment for this sportello */
        int assigned = 0;
        sportello_t my = {0};

        while (1) {
            /* try to receive assignment only if not assigned yet */
            if (!assigned) {
                sportello_msg_t msg;
                ssize_t r = msgrcv(msg_id, &msg, sizeof(sportello_msg_t) - sizeof(long), 1000 + spor_id, IPC_NOWAIT);
                if (r != -1) {
                    my = msg.sportello_info;
                    assigned = 1;
                } else if (errno != ENOMSG) {
                    perror("msgrcv sportello");
                    exit(EXIT_FAILURE);
                }
            }

            /* end-of-simulation during the day? */
            int t = sv_sem_trywait(sem_id, 2);
            if (t == 1) return;
            if (t == -1) { perror("sv_sem_trywait sem2 (sportello)"); exit(EXIT_FAILURE); }

            /* end-of-day? -> next outer day */
            int e = sv_sem_trywait(sem_id, 1);
            if (e == 1) break;
            if (e == -1) { perror("sv_sem_trywait sem1 (sportello)"); exit(EXIT_FAILURE); }

            /* no event: be polite to scheduler */
            sched_yield();
        }
        /* next day: the director will send a new assignment; assigned resets */
    }
}


int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    //Init logger
    int log_qid = open_log_queue();

    //Init sem
    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 4, 0);
    if (sem_id == -1) { perror("semget"); exit(EXIT_FAILURE); }

    //Init sportello mq
    key_t mq_key = get_queue_key(FTOK_PATH_SPOR, MSG_QUEUE_ID_SPOR);
    int   msg_id = init_msg_queue(mq_key);

    int spor_id = atoi(argv[1]);

    sv_sem_signal(sem_id, 3);
    receive_service(spor_id, sem_id, log_qid, msg_id);

    return 0;
}