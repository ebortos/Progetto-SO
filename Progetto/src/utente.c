#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/sem.h>
#include <sched.h>
#include <string.h>

/* invia richiesta ticket */
static int send_ticket_request(int erog_qid, pid_t pid, int service_type, int log_qid) {
    erogatore_request_msg req;
    req.mtype        = MTYPE_REQUEST;
    req.service_type = service_type;
    req.pid          = pid;
    req.ticket_number = 0;

    if (msgsnd(erog_qid, &req, sizeof(req) - sizeof(long), 0) == -1) {
        perror("msgsnd (utente->erogatore)");
        return -1;
    }

    log_sendf(log_qid, "[UTENTE %d] Richiesta inviata per servizio %d\n", (int)pid, service_type);
    return 0;
}

/* prova a ricevere la reply per questo pid (non bloccante) */
static int try_receive_reply(int erog_qid, pid_t pid, int *ticket_out, int *service_out) {
    erogatore_reply_msg rep;
    ssize_t r = msgrcv(erog_qid, &rep, sizeof(rep) - sizeof(long), pid, IPC_NOWAIT);

    if (r == -1) {
        if (errno == ENOMSG) return 0;
        perror("msgrcv (reply utente)");
        exit(EXIT_FAILURE);
    }

    if (ticket_out) *ticket_out = rep.ticket_number;
    if (service_out) *service_out = rep.service_type;

    return 1;
}

//utente si mette in coda allo sportello
static int enqueue_service_line(int serv_qid, pid_t pid, int ticket_number, int service_type) {
    erogatore_request_msg s;
    s.mtype         = service_type + 1;   /* routing by service */
    s.service_type  = service_type;
    s.pid           = pid;
    s.ticket_number = ticket_number;

    if (msgsnd(serv_qid, &s, sizeof(s) - sizeof(long), 0) == -1) {
        perror("msgsnd (utente->service line)");
        return -1;
    }

    return 0;
}

/* genera una probabilità p in [p_min, p_max] e decide se andare */
static int utente_rand_decision(float p_min, float p_max) {
    float p = p_min + ((float)rand() / RAND_MAX) * (p_max - p_min);
    float roll = (float)rand() / RAND_MAX;
    return (roll < p) ? 1 : 0;   // 1 = va alle poste, 0 = resta a casa
}

static void run_utente(int sem_id, int erog_qid, int serv_qid, int done_qid, int log_qid, const day_plan_t *plan, float p_min, float p_max) {
    const pid_t me = getpid();
    srand((unsigned)time(NULL) ^ (unsigned)me);

    for (;;) {
        /* 1) start of day */
        sv_sem_wait(sem_id, 0);

        /* end of simulation right after wake? */
        int v = semctl(sem_id, 2, GETVAL);
        if (v == -1) { perror("semctl GETVAL"); exit(EXIT_FAILURE); }
        if (v > 0) return;

        int asked_ticket = 0, got_ticket = 0, queued_sp = 0, served = 0;
        int my_ticket = -1, my_service = -1;

        /* 2) decide whether to go today */
        if (utente_rand_decision(p_min, p_max)) {
            /* pick the service first */
            my_service = rand() % NUM_SERVICES;

            /* check availability for this service today */
            if (plan->counts[my_service] <= 0) {
                log_sendf(log_qid, "[UTENTE %d] NO POSTE (nessuno sportello per servizio %d oggi)\n",
                          (int)me, my_service);
            } else {
                log_sendf(log_qid, "[UTENTE %d] SI POSTE (servizio %d)\n", (int)me, my_service);

                /* ask erogatore for a ticket */
                if (send_ticket_request_erogatore(erog_qid, me, my_service, log_qid) == 0)
                    asked_ticket = 1;
            }
        } else {
            log_sendf(log_qid, "[UTENTE %d] NO POSTE\n", (int)me);
        }

        /* 3) during the day: wait for ticket, then enqueue to sportello, then wait for completion */
        for (;;) {
            int did_something = 0;

            if (asked_ticket && !got_ticket) {
                int tno = -1, st = -1;
                if (try_receive_ticket_from_erogatore(erog_qid, me, &tno, &st) == 1) {
                    my_ticket  = tno;
                    if (st >= 0) my_service = st;  /* trust erogatore’s echo if provided */
                    got_ticket = 1;
                    did_something = 1;

                    /* queue to sportello line for my service */
                    if (enqueue_service_line(serv_qid, me, my_ticket, my_service) == 0)
                        queued_sp = 1;
                }
            }

            if (queued_sp && !served) {
                int fin_tno = -1, fin_st = -1;
                if (try_receive_completion_from_sportello(done_qid, me, &fin_tno, &fin_st) == 1) {
                    served = 1;
                    did_something = 1;
                    log_sendf(log_qid, "[UTENTE %d] Servito (ticket=%d, serv=%d)\n",
                              (int)me, fin_tno, fin_st);
                }
            }

            /* end-of-simulation? */
            int t = sv_sem_trywait(sem_id, 2);
            if (t == 1) return;
            if (t == -1) { perror("sv_sem_trywait sem2 (utente)"); exit(EXIT_FAILURE); }

            /* end-of-day? -> if not served, discard and try again tomorrow */
            int e = sv_sem_trywait(sem_id, 1);
            if (e == 1) break;
            if (e == -1) { perror("sv_sem_trywait sem1 (utente)"); exit(EXIT_FAILURE); }

            if (!did_something) sched_yield();
        }
        /* loop: next day */
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    int log_qid  = open_log_queue();

    /* queues */
    int erog_qid = init_msg_queue(get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG));
    int serv_qid = open_service_queue();
    int done_qid = open_done_queue();

    /* semaphores (0 start,1 stop,2 end sim,3 ready) */
    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 4, 0);
    if (sem_id == -1) { perror("semget"); exit(EXIT_FAILURE); }

    /* attach read-only day plan */
    int plan_shmid = shm_plan_get_existing();
    day_plan_t *plan = shm_plan_attach_ro(plan_shmid);

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <p_min> <p_max>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    float p_min = atof(argv[1]);
    float p_max = atof(argv[2]);
    if (p_min < 0) p_min = 0;
    if (p_max > 1) p_max = 1;
    if (p_min > p_max) { float t = p_min; p_min = p_max; p_max = t; }

    /* ready barrier */
    sv_sem_signal(sem_id, 3);

    run_utente(sem_id, erog_qid, serv_qid, done_qid, plan, log_qid, p_min, p_max);

    shm_plan_detach(plan);
    return 0;
}
