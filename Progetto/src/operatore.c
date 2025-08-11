#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sched.h>

/* duration per service (ms) */
static int service_ms(int s) {
    switch (s) {
        case PACCHI:               return 10;
        case LETTERE:              return 8;
        case BANCOPOSTA:           return 6;
        case BOLLETTINI:           return 8;
        case PRODOTTI_FINANZIARI:  return 20;
        case OROLOGI:              return 20;
        default:                   return 5;
    }
}

/* serve a job but allow interrupts from end-of-day (sem1) or end-of-sim (sem2)
   returns 1 if fully served, 0 if aborted */
static int serve_job_interruptible(int sem_id, int service_type, const long NANOS_SIM_MIN) {
    int remaining = service_ms(service_type);

    while (remaining > 0) {
        // end-of-simulation?
        int t = sv_sem_trywait(sem_id, 2);
        if (t == 1) return 0;
        if (t == -1) { perror("sv_sem_trywait sem2 (operatore)"); exit(EXIT_FAILURE); }

        // end-of-day?
        int e = sv_sem_trywait(sem_id, 1);
        if (e == 1) return 0;
        if (e == -1) { perror("sv_sem_trywait sem1 (operatore)"); exit(EXIT_FAILURE); }

        // sleep one simulated minute
        struct timespec ts = {
            .tv_sec  = NANOS_SIM_MIN / 1000000000L,
            .tv_nsec = NANOS_SIM_MIN % 1000000000L
        };
        // handle EINTR so partial sleeps don’t extend the service
        while (nanosleep(&ts, &ts) == -1 && errno == EINTR) { /* retry remaining */ }

        --remaining;
    }
    return 1;
}


static void run_operatore(int sem_id, int serv_qid, int done_qid, int log_qid, int my_service, const long NANOS_SIM_MIN)
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
            ssize_t r = msgrcv(serv_qid, &req, MSGSZ(erogatore_request_msg), (long)(my_service + 1), IPC_NOWAIT);

            if (r != -1) {
                /* try to serve; may be aborted by end-of-day/sim */
                int ok = serve_job_interruptible(sem_id, req.service_type, NANOS_SIM_MIN);

                if (ok) {
                    /* completion goes to user's inbox (mtype = pid) */
                    erogatore_reply_msg done = {0};
                    done.mtype         = req.pid;
                    done.ticket_number = req.ticket_number;
                    done.service_type  = req.service_type;

                    if (msgsnd(done_qid, &done, MSGSZ(erogatore_reply_msg), 0) == -1) {
                        perror("msgsnd (operatore->utente done)");
                        exit(EXIT_FAILURE);
                    }

                    //log_sendf(log_qid, "[OPERATORE %d] Completato ticket=%d serv=%d per PID %d\n", (int)me, req.ticket_number, req.service_type, (int)req.pid);
                } else {
                    /* interrupted: do NOT send completion */
                    log_sendf(log_qid, "[OPERATORE %d] Interrotto (serv=%d, pid=%d)\n", (int)me, req.service_type, (int)req.pid);
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
    long NANOS_SIM_MIN  = strtoll(argv[2], NULL, 10);

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

    run_operatore(sem_id, serv_qid, done_qid, log_qid, my_service, NANOS_SIM_MIN);
    return 0;
}
