#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sched.h>

/* duration per service (ms) */
static int service_ms(int s) {
    switch (s) {
        case PACCHI:               return 90;
        case LETTERE:              return 60;
        case BANCOPOSTA:           return 120;
        case BOLLETTINI:           return 80;
        case PRODOTTI_FINANZIARI:  return 150;
        case OROLOGI:              return 70;
        default:                   return 75;
    }
}

/* serve a job but allow interrupts from end-of-day (sem1) or end-of-sim (sem2)
   returns 1 if fully served, 0 if aborted */
static int serve_job_interruptible(int sem_id, int service_type) {
    int remaining_ms = service_ms(service_type);
    const int slice_ms = 10;  /* check every 10ms */

    while (remaining_ms > 0) {
        /* end-of-simulation? */
        int t = sv_sem_trywait(sem_id, 2);
        if (t == 1) return 0;                   /* abort */
        if (t == -1) { perror("sv_sem_trywait sem2 (operatore)"); exit(EXIT_FAILURE); }

        /* end-of-day? */
        int e = sv_sem_trywait(sem_id, 1);
        if (e == 1) return 0;                   /* abort */
        if (e == -1) { perror("sv_sem_trywait sem1 (operatore)"); exit(EXIT_FAILURE); }

        int chunk = (remaining_ms > slice_ms) ? slice_ms : remaining_ms;
        struct timespec ts = { .tv_sec = 0, .tv_nsec = chunk * 1000000L };
        nanosleep(&ts, NULL);
        remaining_ms -= chunk;
    }
    return 1;
}

static void run_operatore(int sem_id, int serv_qid, int done_qid, int log_qid, int my_service)
{
    const pid_t me = getpid();

    while (1) {
        /* 1) start-of-day (blocking) */
        sv_sem_wait(sem_id, 0);

        /* sim ended right after start? (don’t consume sem2) */
        int v = semctl(sem_id, 2, GETVAL);
        if (v == -1) { perror("semctl GETVAL"); exit(EXIT_FAILURE); }
        if (v > 0) return;

        /* 2) day loop: handle only my service until end-of-day */
        while (1) {
            erogatore_request_msg req;
            ssize_t r = msgrcv(serv_qid,
                               &req,
                               sizeof(req) - sizeof(long),
                               (long)(my_service + 1),   /* queue routed by service */
                               IPC_NOWAIT);

            if (r != -1) {
                /* try to serve; may be aborted by end-of-day/sim */
                int ok = serve_job_interruptible(sem_id, req.service_type);

                if (ok) {
                    /* completion goes to user's inbox (mtype = pid) */
                    erogatore_reply_msg done = {
                        .mtype         = req.pid,
                        .ticket_number = req.ticket_number,
                        .service_type  = req.service_type
                    };
                    if (msgsnd(done_qid, &done, sizeof(done) - sizeof(long), 0) == -1) {
                        perror("msgsnd (operatore->utente done)");
                        exit(EXIT_FAILURE);
                    }

                    log_sendf(log_qid,
                              "[OPERATORE %d] Completato ticket=%d serv=%d per PID %d\n",
                              (int)me, req.ticket_number, req.service_type, (int)req.pid);
                } else {
                    /* interrupted: do NOT send completion */
                    log_sendf(log_qid,
                              "[OPERATORE %d] Interrotto (serv=%d, pid=%d)\n",
                              (int)me, req.service_type, (int)req.pid);
                }

            } else if (errno != ENOMSG) {
                perror("msgrcv (serv_q)");
                exit(EXIT_FAILURE);
            }

            /* end-of-sim during the day? */
            int t = sv_sem_trywait(sem_id, 2);
            if (t == 1) return;
            if (t == -1) { perror("sv_sem_trywait sem2 (operatore)"); exit(EXIT_FAILURE); }

            /* end-of-day? -> new day */
            int e = sv_sem_trywait(sem_id, 1);
            if (e == 1) break;
            if (e == -1) { perror("sv_sem_trywait sem1 (operatore)"); exit(EXIT_FAILURE); }

            /* idle politely when no job */
            if (r == -1) sched_yield();
        }
        /* next day… */
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    int my_service = atoi(argv[1]);

    /* semaphores (0 start,1 stop,2 end sim,3 ready) */
    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 4, 0);
    if (sem_id == -1) { perror("semget"); return EXIT_FAILURE; }

    /* queues */
    int log_qid  = open_log_queue();
    int serv_qid = open_service_queue();
    int done_qid = open_done_queue();

    /* ready barrier */
    sv_sem_signal(sem_id, 3);

    run_operatore(sem_id, serv_qid, done_qid, log_qid, my_service);
    return 0;
}
