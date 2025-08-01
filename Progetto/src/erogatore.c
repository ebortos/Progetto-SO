#define _GNU_SOURCE

#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

extern volatile sig_atomic_t day_start;
extern volatile sig_atomic_t day_end;

void handle_day_start(int sig) { day_start = 1; day_end = 0; }
void handle_day_end(int sig) { day_end = 1; }

int issue_ticket(int sem_id, int msg_id) {
    int ticket_counter = 1;

    while (1) {
        // Aspetta inizio giornata (semaforo 0)
        sem_wait(sem_id, 0);
        printf("[EROGATORE] Inizio giornata lavorativa\n");

        // Ciclo giornaliero: gestisci messaggi finché arriva semaforo fine giornata (semaforo 1)
        int day_over = 0;
        while (!day_over) {
            // Provo a leggere messaggi con IPC_NOWAIT (non bloccante)
            erogatore_request_msg req;
            ssize_t r = msgrcv(msg_id, &req, sizeof(req) - sizeof(long), MTYPE_REQUEST, IPC_NOWAIT);
            if (r == -1) {
                if (errno == ENOMSG) {
                    // Nessun messaggio: controllo se il giorno è finito con semctl
                    struct semid_ds sem_info;
                    if (semctl(sem_id, 1, IPC_STAT, &sem_info) == -1) {
                        perror("semctl IPC_STAT");
                        exit(EXIT_FAILURE);
                    }
                    // Se il semaforo 1 è a zero, significa che direttore ha già fatto sem_signal per fine giornata
                    // quindi esco dal ciclo
                    unsigned short sem_values[2];
                    if (semctl(sem_id, 0, GETALL, sem_values) == -1) {
                        perror("semctl GETALL");
                        exit(EXIT_FAILURE);
                    }
                    // Se il semaforo 1 è > 0 il giorno è finito (sem_signal fatta)
                    // Ma attenzione: sistema System V semafori conta le risorse disponibili.
                    // Più sicuro: Provo un sem_trywait (semop con flag IPC_NOWAIT) sul semaforo 1 per capire se è stato segnato.
                    struct sembuf op = {1, -1, IPC_NOWAIT};
                    if (semop(sem_id, &op, 1) == -1) {
                        if (errno == EAGAIN) {
                            // Semaforo non disponibile -> giorno ancora attivo
                            usleep(50000); // attesa breve per non fare busy wait intenso
                            continue;
                        } else {
                            perror("semop trywait sem1");
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        // Semaforo 1 acquisito -> giorno finito
                        day_over = 1;
                        // Rilascio subito semaforo 1 per non consumarlo
                        struct sembuf op_rel = {1, 1, 0};
                        if (semop(sem_id, &op_rel, 1) == -1) {
                            perror("semop release sem1");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    }
                } else {
                    perror("msgrcv");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Ho ricevuto richiesta, la processo
                printf("[EROGATORE] Ricevuta richiesta per servizio %d dall'utente %d\n", req.service_type, req.pid);

                erogatore_reply_msg reply;
                reply.mtype = req.pid;
                reply.ticket_number = ticket_counter++;

                if (msgsnd(msg_id, &reply, sizeof(reply) - sizeof(long), 0) == -1) {
                    perror("msgsnd (reply) failed");
                    exit(EXIT_FAILURE);
                }
            }
        }

        printf("[EROGATORE] Fine giornata, pausa...\n");
    }

    return 0;
}



int main(int argc, char *argv[]) {
    key_t key = get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    int msg_id = init_msg_queue(key);

    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = semget(sem_key, 2, 0);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    printf("[EROGATORE] Inizializzato e pronto a lavorare (PID %d)\n", getpid());

    issue_ticket(sem_id, msg_id);

    return 0;
}
