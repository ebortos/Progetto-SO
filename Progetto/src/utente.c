#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/sem.h>

//invia una richiesta di ticket per un dato servizio.
int send_ticket_request(int msg_id, pid_t pid, int service_type, int log_qid) {
    erogatore_request_msg req;
    req.mtype        = MTYPE_REQUEST;
    req.service_type = service_type;
    req.pid          = pid;

    if (msgsnd(msg_id, &req, sizeof(req) - sizeof(long), 0) == -1) {
        perror("msgsnd (utente->erogatore)");
        return -1;
    }

    log_sendf(log_qid, "[UTENTE %d] Richiesta inviata per servizio %d\n", pid, service_type);
    return 0;
}

/* Prova a ricevere (NON bloccante) la risposta per questo pid.
   Se ricevuta: scrive il ticket in *ticket_out e ritorna 1.
   Se non c'è ancora: ritorna 0.
   Se errore vero: stampa e termina. */
int try_receive_reply(int msg_id, pid_t pid, int *ticket_out) {
    erogatore_reply_msg rep;
    ssize_t r = msgrcv(msg_id, &rep, sizeof(rep) - sizeof(long), pid, IPC_NOWAIT);
    if (r == -1) {
        if (errno == ENOMSG) return 0;   // nessuna risposta (ancora)
        perror("msgrcv (reply utente)");
        exit(EXIT_FAILURE);
    }
    if (ticket_out) *ticket_out = rep.ticket_number;
    return 1;
}

//se l'utente vuole andare alle poste ritorna 1
int utente_rand_decision(float p_serv_min, float p_serv_max) {
    float p_serv = p_serv_min + ((float)rand() / RAND_MAX) * (p_serv_max - p_serv_min);
    float roll = (float)rand() / RAND_MAX;
    return (roll < p_serv) ? 1 : 0;
}

static void run_utente(int sem_id, int msg_id, int log_qid, int p_serv_min, int p_serv_max) {
    const pid_t me = getpid();
    int request_sent = 0;      /* inviamo UNA richiesta totale */
    int my_service   = -1;     /* memorizzo il servizio richiesto */
    int my_ticket    = -1;

    srand(time(NULL) ^ me);

    while (1) {
        /* 1) attesa inizio giornata (bloccante) */
        sv_sem_wait(sem_id, 0);

        if(utente_rand_decision(p_serv_min, p_serv_max)==1){
            log_sendf(log_qid, "[UTENTE %d] oggi non vado alle poste troppo sbatti\n", me, my_ticket);
            break;
        }
        else
            log_sendf(log_qid, "[UTENTE %d] sai che c'e', oggi ci vado alle poste\n", me, my_ticket);
        //decidere se avvertire quando l'utente non si reca alle poste

        /* subito dopo: fine simulazione? (non consumiamo sem2) */
        int v = semctl(sem_id, 2, GETVAL);

        if (v == -1) { 
            perror("semctl GETVAL"); 
            exit(EXIT_FAILURE); 
        }

        if (v > 0) return; //fine sim

        /* 2) invia la richiesta una volta sola (se non ancora inviata) */
        if (!request_sent) {
            my_service = rand() % 6;

            if (send_ticket_request(msg_id, me, my_service, log_qid) == -1)
                exit(EXIT_FAILURE);
            request_sent = 1;
        }

        /* 3) durante la giornata: aspetto la risposta, o fine giornata/simulazione */
        while (1) {
            /* provo a ricevere la risposta */
            if (try_receive_reply(msg_id, me, &my_ticket) == 1) {
                log_sendf(log_qid, "[UTENTE %d] Ricevuto ticket: %d\n", me, my_ticket);
                return;  /* lavoro utente concluso */
            }

            /* fine simulazione durante la giornata? */
            int t = sv_sem_trywait(sem_id, 2);

            if (t == 1) return;

            else if (t == -1) {
                perror("sem_trywait sem2 (utente)");
                exit(EXIT_FAILURE);
            }

            /* fine giornata? -> aspetto il giorno successivo e continuo ad attendere la reply */
            int e = sv_sem_trywait(sem_id, 1);

            if (e == 1) break;  /* esce dal for, tornerà a sem_wait(sem0) */
            
            else if (e == -1) {
                perror("sem_trywait sem1 (utente)");
                exit(EXIT_FAILURE);
            }

            /* nessun evento: piccola attesa non aggressiva */
            usleep(30000);  /* 30 ms */
        }
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);  /* stdout line-buffered per debug */

    int log_qid = open_log_queue();

    /* coda messaggi (stessa dell'erogatore) */
    key_t mq_key = get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    int   msg_id = init_msg_queue(mq_key);

    /* semafori (0 start, 1 stop, 2 end sim, 3 ready) */
    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    if (sem_key == -1) { 
        perror("ftok sem"); 
        exit(EXIT_FAILURE); 
    }

    int sem_id = semget(sem_key, 4, 0);   /* creati dal direttore */
    if (sem_id == -1) { 
        perror("semget"); 
        exit(EXIT_FAILURE); 
    }

    //ready
    sv_sem_signal(sem_id, 3);
    //passagio dei valori per il random della giornata
    //presi da config/direttore/argv utente 
    if(argc < 3){
        perror("min, max utente");
        exit(EXIT_FAILURE);
    }
    float p_serv_min = atof(argv[1]), p_serv_max = atof(argv[2]);
    run_utente(sem_id, msg_id, log_qid, p_serv_min, p_serv_max);

    return 0;
}