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
static int serve_job(int sem_id, int service_type, const long NANOS_SIM_MIN) {
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

//operatore prova ad occupare uno sportello
static inline int seat_try_acquire(int seats_sid, int s) {
    struct sembuf op = { .sem_num = (unsigned short)s, .sem_op = -1, .sem_flg = IPC_NOWAIT };
    if (semop(seats_sid, &op, 1) == 0) return 1;

    if (errno == EAGAIN) return 0;
    perror("semop seat_try_acquire"); exit(EXIT_FAILURE);
}

//operatore lascia lo sportello per andare in pausa
static inline void seat_release(int seats_sid, int s) {
    struct sembuf op = { .sem_num = (unsigned short)s, .sem_op = +1, .sem_flg = 0 };
    if (semop(seats_sid, &op, 1) == -1) { perror("semop seat_release"); exit(EXIT_FAILURE); }
}

// Return 1 if we should pause now (policy), 0 otherwise.
static inline int should_pause_today(int pauses_left) {
    if (pauses_left <= 0) return 0;

    int roll = rand() % 100;
    // DEBUG:
    fprintf(stderr, "[PAUSE?]pauses_left=%d roll=%d\n", pauses_left, roll);

    return roll < 20; // 20% default
}

// Returns:
//   0 => pause completed, seat re-acquired: keep working this day
//   1 => end-of-day happened during pause/wait: caller should ACK sem3 and go next day
//   2 => end-of-sim happened: caller should exit
static int try_short_pause(int sem_id, int seats_sid, int my_service, long NANOS_SIM_MIN, int pause_minutes, int *has_seat, int *pauses_left, int log_qid) {
    if (!should_pause_today(*pauses_left)) return 0;

    (*pauses_left)--;
    //log_sendf(log_qid, "[OPERATORE %d] Pausa breve (serv=%d, pause rimaste=%d)\n", (int)getpid(), my_service, *pauses_left);

    // Release seat (so others can take over)
    if (*has_seat) {
        struct sembuf rel = { .sem_num = (unsigned short)my_service, .sem_op = +1, .sem_flg = 0 };
        if (semop(seats_sid, &rel, 1) == -1) { perror("seat_release"); exit(EXIT_FAILURE); }
        *has_seat = 0;
    }

    // Pause for pause_minutes "simulated minutes", responsive to day/sim end
    for (int i = 0; i < pause_minutes; ++i) {
        int t = sv_sem_trywait(sem_id, 2); // end-of-sim?
        if (t == 1) return 2;
        int e = sv_sem_trywait(sem_id, 1); // end-of-day?
        if (e == 1) return 1;

        struct timespec ts = {
            .tv_sec  = NANOS_SIM_MIN / 1000000000L,
            .tv_nsec = NANOS_SIM_MIN % 1000000000L
        };
        while (nanosleep(&ts, &ts) == -1 && errno == EINTR) { /* retry */ }
    }

    // Try to reacquire a seat; wait politely, but bail if day/sim ends
    for (;;) {
        struct sembuf acq = { .sem_num = (unsigned short)my_service, .sem_op = -1, .sem_flg = IPC_NOWAIT };
        if (semop(seats_sid, &acq, 1) == 0) { *has_seat = 1; return 0; }

        if (errno != EAGAIN) { perror("seat_try_acquire"); exit(EXIT_FAILURE); }

        int t = sv_sem_trywait(sem_id, 2); if (t == 1) return 2;
        int e = sv_sem_trywait(sem_id, 1); if (e == 1) return 1;

        struct timespec backoff = {0, 1000000}; // 1 ms
        nanosleep(&backoff, NULL);
    }
}

static void run_operatore(int sem_id, int serv_qid, int done_qid, int log_qid, int my_service, const long NANOS_SIM_MIN, int seats_sid, int *pauses_left_ptr) {
    const pid_t me = getpid();

    const int PAUSE_MINUTES = 10;

    while (1) {
        /* 1) start-of-day (blocking) */
        sv_sem_wait(sem_id, 0);

        /* sim ended right after start? (don’t consume sem2) */
        int v = semctl(sem_id, 2, GETVAL);
        if (v == -1) { perror("semctl GETVAL"); exit(EXIT_FAILURE); }
        if (v > 0) return;

        int served_today = 0;
        int has_seat     = 0;

        while (!has_seat) {
            if (seat_try_acquire(seats_sid, my_service)) {
                has_seat = 1;
                break;
            }
            /* end-of-sim while waiting? */
            int t = sv_sem_trywait(sem_id, 2);
            if (t == 1) return;
            if (t == -1) { perror("sv_sem_trywait sem2 (operatore)"); exit(EXIT_FAILURE); }

            /* end-of-day while waiting? ack and go next day */
            int e = sv_sem_trywait(sem_id, 1);
            if (e == 1) { sv_sem_signal(sem_id, 3); goto next_day; }
            if (e == -1) { perror("sv_sem_trywait sem1 (operatore)"); exit(EXIT_FAILURE); }

            /* small backoff to avoid spinning */
            struct timespec ts = {0, 1000000};  // 1 ms
            nanosleep(&ts, NULL);
        }

        /* 3) day loop: handle only my service until end-of-day */
        for (;;) {
            erogatore_request_msg req;
            ssize_t r = msgrcv(serv_qid, &req, MSGSZ(erogatore_request_msg), (long)(my_service + 1), IPC_NOWAIT);

            if (r != -1) {
                /* try to serve; may be aborted by end-of-day/sim */
                int ok = serve_job(sem_id, req.service_type, NANOS_SIM_MIN);

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

                    /* maybe pause for the rest of the day */
                    int pause_res = try_short_pause(sem_id, seats_sid, my_service, NANOS_SIM_MIN, PAUSE_MINUTES, &has_seat, pauses_left_ptr, log_qid);

                    if (pause_res == 1) {                 // day ended during pause/reacquire
                        if (has_seat) {                   // (usually false, but be safe)
                            seat_release(seats_sid, my_service);
                            has_seat = 0;
                        }

                        sv_sem_signal(sem_id, 3);         // day-end ACK
                        goto next_day;
                    }

                    if (pause_res == 2) {                 // simulation ended
                        if (has_seat) seat_release(seats_sid, my_service);
                            return;
                    }
                } else {
                    /* interrupted: do NOT send completion */
                    log_sendf(log_qid, "[OPERATORE %d] Interrotto (serv=%d, pid=%d)\n",
                              (int)me, req.service_type, (int)req.pid);
                }

            } else if (errno != ENOMSG) {
                perror("msgrcv (serv_q)");
                exit(EXIT_FAILURE);
            }

            /* end-of-sim during the day? */
            {
                int t = sv_sem_trywait(sem_id, 2);
                if (t == 1) {
                    if (has_seat) seat_release(seats_sid, my_service);
                    return;
                }
                if (t == -1) { perror("sv_sem_trywait sem2 (operatore)"); exit(EXIT_FAILURE); }
            }

            /* end-of-day? -> release seat, ACK and go next day */
            {
                int e = sv_sem_trywait(sem_id, 1);
                if (e == 1) {
                    if (has_seat) { seat_release(seats_sid, my_service); has_seat = 0; }
                    sv_sem_signal(sem_id, 3); /* end-of-day arrival */
                    break;
                }
                if (e == -1) { perror("sv_sem_trywait sem1 (operatore)"); exit(EXIT_FAILURE); }
            }

            /* idle politely when no job */
            if (r == -1) {
                struct timespec ts = {0, 1000000};  // 1 ms
                nanosleep(&ts, NULL);
            }
        }
    next_day:
        ; /* loop to next day */
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    srand(time(NULL) ^ getpid());

    int my_service = atoi(argv[1]);
    long NANOS_SIM_MIN  = strtoll(argv[2], NULL, 10);
    int pauses_left = atoi(argv[3]);

    /* semaphores (0 start,1 stop,2 end sim,3 ready) */
    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 4, 0);
    if (sem_id == -1) { perror("semget"); return EXIT_FAILURE; }

    key_t seats_key = ftok(FTOK_PATH_SEATS, SEM_SEATS_ID);
    int seats_sid = semget(seats_key, NUM_SERVICES, 0);
    if (seats_sid == -1) { perror("semget seats"); return EXIT_FAILURE; }

    /* queues */
    int log_qid  = open_log_queue();
    int serv_qid = open_service_queue();
    int done_qid = open_done_queue();

    /* ready barrier */
    sv_sem_signal(sem_id, 3);

    run_operatore(sem_id, serv_qid, done_qid, log_qid, my_service, NANOS_SIM_MIN, seats_sid, &pauses_left);
    return 0;
}
