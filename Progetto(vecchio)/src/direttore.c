//AGGIUSTARE STATISTICHE
//AGGIUSTARE SINCRONIZZAZIONE PROCESSI!!!!!!!!!!!
//AGGIUSTARE CONFIG
//aggiustare file di stampa


#include "../include/ipc_utils.h"
#include "../include/sportello.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>

#define _GNU_SOURCE
#define MSG_KEY 1234
#define MAX_SERVICES 6

typedef struct {
    float avg_wait_time;
    int total_users_served;
    int active_workers;
    int users_left_waiting;
} DailyStats;

typedef struct {
    int nof_workers;
    int nof_users;
    int sim_duration;
    int explode_threshold;
    int nof_counters;
    int n_nano_secs;
    key_t shm_key;
    key_t sem_key;
} Config;

typedef struct {
    int id_operatore;
    int servizio_assegnato;
} Operatore;

typedef struct {
    long mtype; // Message type
    DailyStats stats; // Daily statistics
} Message;

void read_config(Config *config) {
    FILE *file = fopen("../config.txt", "r");
    if (!file) {
        perror("Error opening config file");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        sscanf(buffer, "NOF_WORKERS=%d", &config->nof_workers);
        sscanf(buffer, "NOF_USERS=%d", &config->nof_users);
        sscanf(buffer, "SIM_DURATION=%d", &config->sim_duration);
        sscanf(buffer, "EXPLODE_THRESHOLD=%d", &config->explode_threshold);
        sscanf(buffer, "NOF_COUNTERS=%d", &config->nof_counters);
        sscanf(buffer, "N_NANO_SECS=%d", &config->n_nano_secs);
        sscanf(buffer, "SHM_KEY=%x", &config->shm_key);
        sscanf(buffer, "SEM_KEY=%x", &config->sem_key);
    }

    fclose(file);
}

void log_stats_to_csv(FILE *csv_file, int day, DailyStats *stats) {
    fprintf(csv_file, "%d,%.2f,%d,%d\n", day, stats->avg_wait_time, stats->total_users_served, stats->users_left_waiting);
}

void assign_services_to_counters(int *counters, int nof_workers) {
    printf("Assegnazione dei servizi agli sportelli...\n");
    for (int i = 0; i < nof_workers; i++) {
        counters[i] = rand() % MAX_SERVICES;
        printf("Sportello %d assegnato al servizio %d\n", i, counters[i]);
    }
}

void assign_service_to_operators(Operatore *operatori, int nof_workers) {
    for (int i = 0; i < nof_workers; i++) {
        operatori[i].id_operatore = i;
        operatori[i].servizio_assegnato = i % MAX_SERVICES;
        printf("Operatore %d assegnato al servizio %d\n", i, operatori[i].servizio_assegnato);
    }
}

void notify_operators(int nof_workers) {
    printf("Notifica agli operatori di iniziare...\n");
    for (int i = 0; i < nof_workers; i++) {
        kill(0, SIGUSR1);
    }
}

void handle_termination(int cause, int total_days, float total_avg_wait_time, int total_users_served, int total_users_left_waiting) {
    printf("Simulazione terminata. Causa: ");
    if (cause == 1) {
        printf("Timeout (SIM_DURATION raggiunta).\n");
    } else if (cause == 2) {
        printf("Troppi utenti in attesa (EXPLODE_THRESHOLD superato).\n");
    }

    printf("Statistiche finali:\n");
    printf("Giorni totali: %d\n", total_days);
    printf("Tempo medio totale di attesa: %.2f sec\n", total_avg_wait_time / total_days);
    printf("Utenti totali serviti: %d\n", total_users_served);
    printf("Utenti rimasti in attesa: %d\n", total_users_left_waiting);

    exit(EXIT_SUCCESS);
}

int main() {
    printf(">> Processo Direttore Avviato...\n");

    Config config;
    read_config(&config);

    Operatore *operatori = malloc(config.nof_workers * sizeof(Operatore));
    if (operatori == NULL) {
        perror("Errore nell'allocazione dinamica della memoria per gli operatori");
        exit(EXIT_FAILURE);
    }

    key_t director_key = ftok("../config.txt", 'D');
    if (director_key == -1) {
        perror("Error generating director key");
        exit(EXIT_FAILURE);
    }

    int sem_id = create_semaphore(config.sem_key, config.nof_counters);
    for (int i = 0; i < config.nof_counters; i++) {
        if (semctl(sem_id, i, SETVAL, 1) == -1) {
            perror("Errore inizializzazione semaforo");
            exit(EXIT_FAILURE);
        }
    }

    int msg_queue_id = init_message_queue(MSG_KEY);

    assign_service_to_operators(operatori, config.nof_workers);

    //////////////////////////////////////////////////////
    FILE *csv_file = fopen("../stats.csv", "w");
    if (!csv_file) {
        perror("Error opening CSV file");
        exit(EXIT_FAILURE);
    }
    fprintf(csv_file, "Day,AvgWaitTime,TotalUsersServed,UsersLeftWaiting\n");

    inizializza_sportelli(config.nof_counters, config.sem_key);

    float total_avg_wait_time = 0;
    int total_users_served = 0;
    int total_users_left_waiting = 0;

    char msg_key_str[16];
    char director_key_str[16];
    char n_nano_secs_str[16];
    char servizio_assegnato_str[16];
    char shm_key_str[16];
    char sem_key_str[16];

    sprintf(msg_key_str, "%d", MSG_KEY);
    sprintf(director_key_str, "%d", director_key);
    sprintf(n_nano_secs_str, "%d", config.n_nano_secs);
    sprintf(servizio_assegnato_str, "%d", operatori->servizio_assegnato);
    sprintf(shm_key_str, "%d", config.shm_key);
    sprintf(sem_key_str, "%d", config.sem_key);

    if (fork() == 0) {
        if (execl("./erogatore", "ticket_dispenser", msg_key_str, director_key_str, NULL) == -1) {
        perror("Error execl ticket_dispenser");
        exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < config.nof_workers; i++) {
        if (fork() == 0) {
            if (execl("./operatore", "operator", msg_key_str, director_key_str, servizio_assegnato_str, shm_key_str, sem_key_str, NULL) == -1) {
            perror("Error execl operator");
            exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 0; i < config.nof_users; i++) {
        char user_id_str[16];
        sprintf(user_id_str, "%d", i + 1);
        
        if (fork() == 0) {
            execl("./utente", "user", msg_key_str, director_key_str, user_id_str, n_nano_secs_str, NULL);
            perror("Errore execl user");
            exit(EXIT_FAILURE);
        }
    }

    int *counters = malloc(config.nof_workers * sizeof(int));
    if (!counters) {
        perror("Error allocating memory for counters");
        exit(EXIT_FAILURE);
    }

    struct timespec sleep_time = {0};
    sleep_time.tv_nsec = config.n_nano_secs;

    printf("\n-------Simulation started-------\n");

    for (int day = 0; day < config.sim_duration; day++) {
        printf("-------Giornata %d iniziata---------\n", day + 1);

        assign_services_to_counters(counters, config.nof_workers);

        //notify_operators(config.nof_workers);

        nanosleep(&sleep_time, NULL);

        int users_waiting_end_of_day = 0;
        DailyStats stats;

        while (msgrcv(msg_queue_id, &stats, sizeof(DailyStats), 0, IPC_NOWAIT) != -1) {
            printf("Ricevute statistiche: Attesa media %.2f sec, Utenti serviti %d, Utenti in attesa %d.\n",
                   stats.avg_wait_time, stats.total_users_served, stats.users_left_waiting);

            total_avg_wait_time += stats.avg_wait_time;
            total_users_served += stats.total_users_served;
            users_waiting_end_of_day += stats.users_left_waiting;

            log_stats_to_csv(csv_file, day + 1, &stats);
        }

        for (int i = 0; i < config.nof_counters; i++) {
            libera_sportello(i);
        }

        total_users_left_waiting += users_waiting_end_of_day;

        if (users_waiting_end_of_day > config.explode_threshold) {
            distruggi_sportelli();
            handle_termination(2, day + 1, total_avg_wait_time, total_users_served, total_users_left_waiting);
        }
    }

    free(operatori);
    free(counters);
    distruggi_sportelli();
    handle_termination(1, config.sim_duration, total_avg_wait_time, total_users_served, total_users_left_waiting);

    fclose(csv_file);
    printf("\n-------Simulation complete.-------\n");
    return 0;
}
