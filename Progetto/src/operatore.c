#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/sem.h>

static void run_operatore(int sem_id, int msg_id, int log_qid) {
    log_sendf(log_qid, "[OPERATORE] pronto per il plug anale mr. direttore ;P");
}

int main(int argc, char *argv[]) {
    //ci sono argomenti da passare?

    setvbuf(stdout, NULL, _IOLBF, 0);
    //teoricamente non serve?

    srand(time(NULL) ^ getpid());

    int log_qid = open_log_queue();

    key_t mq_key = get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    int msg_id = init_msg_queue(mq_key);

    /* semafori (0 start, 1 stop, 2 end sim, 3 ready) */
    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 4, 0);
    if (sem_id == -1) { perror("semget"); exit(EXIT_FAILURE); }
    //il numero del semaforo è corretto?

    /* ready barrier */
    sv_sem_signal(sem_id, 3);

    run_operatore(sem_id, msg_id, log_qid);

    return 0;
}