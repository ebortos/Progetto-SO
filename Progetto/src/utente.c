#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/sem.h>

/* invia richiesta ticket */
static int send_ticket_request(int msg_id, pid_t pid, int service_type, int log_qid) {
    erogatore_request_msg req;
    req.mtype        = MTYPE_REQUEST;
    req.service_type = service_type;
    req.pid          = pid;

    if (msgsnd(msg_id, &req, sizeof(req) - sizeof(long), 0) == -1) {
        perror("msgsnd (utente->erogatore)");
        return -1;
    }
    log_sendf(log_qid, "[UTENTE %d] Richiesta inviata per servizio %d\n", (int)pid, service_type);
    return 0;
}

/* prova a ricevere la reply per questo pid (non bloccante) */
static int try_receive_reply(int msg_id, pid_t pid, int *ticket_out) {
    erogatore_reply_msg rep;
    ssize_t r = msgrcv(msg_id, &rep, sizeof(rep) - sizeof(long), pid, IPC_NOWAIT);
    if (r == -1) {
        if (errno == ENOMSG) return 0;
        perror("msgrcv (reply utente)");
        exit(EXIT_FAILURE);
    }
    if (ticket_out) *ticket_out = rep.ticket_number;
    return 1;
}

/* genera una probabilità p in [p_min, p_max] e decide se andare */
static int utente_rand_decision(float p_min, float p_max) {
    float p = p_min + ((float)rand() / RAND_MAX) * (p_max - p_min);
    float roll = (float)rand() / RAND_MAX;
    return (roll < p) ? 1 : 0;   // 1 = va alle poste, 0 = resta a casa
}

static void run_utente(int sem_id, int msg_id, int log_qid, float p_min, float p_max) {
    const pid_t me = getpid();
    srand((unsigned)time(NULL) ^ (unsigned)me);

    /* stato che può attraversare più giorni */
    int waiting_reply = 0;   // sto aspettando la reply di una richiesta inviata in un giorno precedente?
    int my_ticket     = -1;

    while (1) {
        /* 1) attesa inizio giornata (bloccante) */
        sv_sem_wait(sem_id, 0);

        /* fine simulazione subito dopo la partenza del giorno? (non consumiamo sem2) */
        int v = semctl(sem_id, 2, GETVAL);
        if (v == -1) {
            perror("semctl GETVAL"); 
            exit(EXIT_FAILURE); 
        }

        if (v > 0) return;

        /* per-giorno */
        int went_today = 0;

        /* se non sto già aspettando una reply da un giorno precedente,
           decido se ANDARE oggi e (al massimo) invio una richiesta */
        if (!waiting_reply) {
            if (utente_rand_decision(p_min, p_max)) {
                int service = rand() % 6;
                log_sendf(log_qid, "[UTENTE %d] SI POSTE\n", (int)me);

                if (send_ticket_request(msg_id, me, service, log_qid) == -1) 
                    exit(EXIT_FAILURE);

                waiting_reply = 1;
                went_today = 1;
            } else {
                /* non vado oggi, ma resto in vita per i prossimi giorni */
                log_sendf(log_qid, "[UTENTE %d] NO POSTE\n", (int)me);
            }
        }

        /* 2) durante la giornata: se sto aspettando una reply, la cerco;
              in ogni caso controllo fine giornata / fine simulazione     */
        while (1) {
            /* prova a ricevere la risposta se la sto aspettando */
            if (waiting_reply) {
                int got = try_receive_reply(msg_id, me, &my_ticket);

                if (got == 1) {
                    log_sendf(log_qid, "[UTENTE %d] Ricevuto ticket: %d\n", (int)me, my_ticket);
                    waiting_reply = 0;   /* libero di eventualmente fare un’altra richiesta domani */
                }
            }

            /* fine simulazione durante la giornata? */
            int t = sv_sem_trywait(sem_id, 2);
            if (t == 1) return;
            else if (t == -1) { perror("sv_sem_trywait sem2 (utente)"); exit(EXIT_FAILURE); }

            /* fine giornata? -> passo al giorno successivo */
            int e = sv_sem_trywait(sem_id, 1);
            if (e == 1) break;         /* esco dal for interno: nuova giornata */
            else if (e == -1) { perror("sv_sem_trywait sem1 (utente)"); exit(EXIT_FAILURE); }

            /* nessun evento: piccola attesa non aggressiva */
            if (!went_today && !waiting_reply) {
                /* non ho niente da fare oggi: riduci il carico CPU */
                usleep(30000);
            } else {
                /* sto aspettando la reply: controlla più spesso */
                usleep(5000);
            }
        }
        /* fine del giorno: si ricomincia il ciclo for(;;) con un nuovo giorno */
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    srand(time(NULL) ^ getpid());

    int log_qid = open_log_queue();

    /* coda messaggi (comune con l’erogatore) */
    key_t mq_key = get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    int   msg_id = init_msg_queue(mq_key);

    /* semafori (0 start, 1 stop, 2 end sim, 3 ready) */
    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 4, 0);
    if (sem_id == -1) { perror("semget"); exit(EXIT_FAILURE); }

    /* argomenti: p_min p_max */
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

    run_utente(sem_id, msg_id, log_qid, p_min, p_max);
    return 0;
}
