#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>


void issue_ticket(int sem_id, int msg_id) {
    int ticket_counter = 1;

    while (1) {     
        sem_wait(sem_id, 0);       //aspetta inizio giornata (sem 0, bloccante)
        printf("[EROGATORE] Inizio giornata lavorativa\n");

        struct sembuf op_check_term = {2, -1, IPC_NOWAIT};  //controlla fine simulazione DOPO fine giornata
        if (semop(sem_id, &op_check_term, 1) != -1) {
            printf("[EROGATORE] Fine simulazione rilevata all'inizio giornata.\n");
            struct sembuf op_rilascia = {2, 1, 0};
            semop(sem_id, &op_rilascia, 1);
            return;
        } else if (errno != EAGAIN) {
            perror("semop controllo fine simulazione");
            exit(EXIT_FAILURE);
        }

        bool active_day = true;

        while (active_day) {       //1. blocca finché arriva una richiesta oppure sem 1 viene segnalato
            erogatore_request_msg request;
            ssize_t r = msgrcv(msg_id, &request, sizeof(request) - sizeof(long), MTYPE_REQUEST, IPC_NOWAIT);

            if (r != -1) {         //ricevuto msg valido, gestisco richiesta
                printf("[EROGATORE] Ricevuta richiesta per servizio %d da PID %d\n", request.service_type, request.pid);

                erogatore_reply_msg reply;
                reply.mtype = request.pid;
                reply.ticket_number = ticket_counter++;

                if (msgsnd(msg_id, &reply, sizeof(reply) - sizeof(long), 0) == -1) {
                    perror("msgsnd");
                    exit(EXIT_FAILURE);
                }
            } else if (errno == ENOMSG) {   //nessun messaggio, controllo terminazione o fine giornata
            
                struct sembuf op_check_term = {2, -1, IPC_NOWAIT};      //2. controlla se simulazione è terminata, sem 2
            
                if (semop(sem_id, &op_check_term, 1) != -1) {
                    printf("[EROGATORE] Terminazione simulazione rilevata.\n");

                    struct sembuf op_rilascia = {2, 1, 0};          //rilascialo subito
                    semop(sem_id, &op_rilascia, 1);
                    return;                                         //esce dal ciclo: termina
                } else if (errno != EAGAIN) {
                    perror("semop fine simulazione erog");
                    exit(EXIT_FAILURE);
                }

                struct sembuf op_check_fine = {1, -1, IPC_NOWAIT};  //3. controllo se la giornata è finita, sem 1
                if (semop(sem_id, &op_check_fine, 1) != -1) {
                    // Giorno terminato
                    struct sembuf op_rilascia = {1, 1, 0};
                    semop(sem_id, &op_rilascia, 1);
                    active_day = false;
                    break;
                } else if (errno != EAGAIN) {
                    perror("semop fine giornata erog");
                    exit(EXIT_FAILURE);
                }
                // 4. Attesa passiva
                pause();

            } else {
                perror("msgrcv erog");
                exit(EXIT_FAILURE);
            }
        }

        printf("[EROGATORE] Fine giornata, pausa...\n");
    }
}



int main(int argc, char *argv[]) {
    key_t key = get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    int msg_id = init_msg_queue(key);

    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 3, 0);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    printf("[EROGATORE] Inizializzato e pronto a lavorare (PID %d)\n", getpid());

    issue_ticket(sem_id, msg_id);

    printf("[EROGATORE] Terminazione completata.\n");

    return 0;
}
