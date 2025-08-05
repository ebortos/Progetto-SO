#define _GNU_SOURCE

#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <time.h>
#include <signal.h>
#include <sys/sem.h>

#define NUM_PROCESSI (1 + config.NOF_USERS + config.NOF_WORKER_SEATS + config.NOF_WORKERS)

typedef struct {
    int NOF_WORKERS;
    int NOF_USERS;
    int NOF_WORKER_SEATS;
    int NOF_PAUSE;
    int SIM_DURATION;
    int N_NANO_SECS;
    float P_SERV_MIN;
    float P_SERV_MAX;
    int EXPLODE_THRESHOLD;
} config_t;

typedef struct {
    pid_t *all_pids;       // array contenente tutti i pid
    size_t n_pids;
} process_table_t;

extern char **environ;     //per execve
process_table_t proc_table;

//legge il config e salva nella struct config_t
int load_config(const char *fname, config_t *config) {
    FILE *file = fopen(fname, "r");
    if (!file) {
        perror("Errore nell'apertura del file.\n");
        return -1;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        char key[64];
        char value[64];

        if (sscanf(line, "%[^=]=%s", key, value) == 2) {
            if (strcmp(key, "NOF_WORKERS") == 0)
                config->NOF_WORKERS = atoi(value);
            else if (strcmp(key, "NOF_USERS") == 0)
                config->NOF_USERS = atoi(value);
            else if (strcmp(key, "NOF_WORKER_SEATS") == 0)
                config->NOF_WORKER_SEATS = atoi(value);
            else if (strcmp(key, "NOF_PAUSE") == 0)
                config->NOF_PAUSE = atoi(value);
            else if (strcmp(key, "SIM_DURATION") == 0)
                config->SIM_DURATION = atoi(value);
            else if (strcmp(key, "N_NANO_SECS") == 0)
                config->N_NANO_SECS = atoi(value);
            else if (strcmp(key, "P_SERV_MIN") == 0)
                config->P_SERV_MIN = atof(value);
            else if (strcmp(key, "P_SERV_MAX") == 0)
                config->P_SERV_MAX = atof(value);
            else if (strcmp(key, "EXPLODE_THRESHOLD") == 0)
                config->EXPLODE_THRESHOLD = atoi(value);
        }
    }

    fclose(file);
    return 0;
}

//PID = PORCO IL DIO

int create_processes(const char *exec_path, int count, pid_t *pid_array, int start_index) {
    char *argv[] = {(char *)exec_path, NULL};

    for (int i = 0; i < count; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (execve(exec_path, argv, environ) == -1) {
                perror("execve");
                exit(EXIT_FAILURE);
            }
        }

        pid_array[start_index + i] = pid;
    }

    return 0;
}

void assign_service_sportello(int msg_id, int num_sportelli) {
    for (int i = 0; i < num_sportelli; i++) {
        sportello_msg_t msg;
        msg.mtype = 1000 + i;       //1000 arbitrario per differenziare i msg
        msg.sportello_info.service_type = rand() % 6;
        msg.sportello_info.occupato = 0;

        if (msgsnd(msg_id, &msg, sizeof(sportello_t), 0) == -1) {
            perror("msgsnd direttore to sportello");
            exit(EXIT_FAILURE);
        }

        printf("[Direttore] Assegnato servizio %d a sportello %d\n", msg.sportello_info.service_type, i);
    }
}

void wait_children_ready(int sem_id, int n_ready) {
    printf("[DIRETTORE]   Attendo %d processi ready …\n", n_ready);

    for (int i = 0; i < n_ready; ++i)         /* blocca finché ognuno non fa +1 */
        sem_wait(sem_id, 3);

    printf("[DIRETTORE]   Tutti i figli sono ready\n");
}

static void sem_broadcast(int sem_id, int sem_num, int count) {
    for (int i = 0; i < count; ++i)
        sem_signal(sem_id, sem_num);
}

void run_simulation(int sim_duration, long n_nano_secs, int sem_id, int n_broadcast) {
    struct timespec day = {
        .tv_sec = n_nano_secs / 1000000000,
        .tv_nsec = n_nano_secs % 1000000000
    };

    for (int d = 1; d <= sim_duration; d++) {
        sem_set(sem_id, 0, 0);  //reset sem0
        sem_set(sem_id, 1, 0);  //reset sem1

        printf("[DIRETTORE] ======== Inizio giorno %d ========\n", d);
        sem_broadcast(sem_id, 0, n_broadcast);    //segnale di inizio giornata, sem 0

        nanosleep(&day, NULL);

        sem_broadcast(sem_id, 1, n_broadcast);    //segnale di fine giornata, sem 1
        printf("[DIRETTORE] ======== Fine giorno %d ==========\n", d);
        //Salvare stats qui (credo)
    }

     /* fine simulazione per TUTTI */
    sem_set(sem_id, 2, 0);
    sem_broadcast(sem_id, 2, n_broadcast);

    /* sveglia chiunque sia fermo su sem0 (di sicurezza) */
    sem_broadcast(sem_id, 0, n_broadcast);

    //Wait children
    for (size_t i = 0; i < proc_table.n_pids; ++i) {
        waitpid(proc_table.all_pids[i], NULL, 0);
    }
}



//**************************************************//
//per ora il main è principalmente usato per testare
//a meno di commenti "DEBUG" lasciare così che dovrebbe andare
//**************************************************//
int main() {
    config_t config;

    setvbuf(stdout, NULL, _IOLBF, 0);   /* stdout line-buffered */

    srand(time(NULL) ^ getpid());

    if (load_config("../config.conf", &config) != 0) {
        perror("Errore nella lettura del file.\n");
        return -1;
    }

    //Init array pid processi
    proc_table.n_pids = 0;
    proc_table.all_pids = malloc(sizeof(pid_t) * proc_table.n_pids);

    if (!proc_table.all_pids) {
        perror("malloc all_pids");
        exit(EXIT_FAILURE);
    }

    //Init semaphores
    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = create_semaphore_set(sem_key, 4);

    //Init msg queue sportello
    key_t spor_queue_key = get_queue_key(FTOK_PATH_SPOR, MSG_QUEUE_ID_SPOR);
    int spor_msg_id = init_msg_queue(spor_queue_key);

    //Erogatore
    create_processes("./erogatore", 1, proc_table.all_pids, proc_table.n_pids);
    proc_table.n_pids += 1;

    //Utente
    create_processes("./utente", config.NOF_USERS, proc_table.all_pids, proc_table.n_pids);
    proc_table.n_pids += config.NOF_USERS;
/*
    //Sportello
    create_processes("./sportello", config.NOF_WORKER_SEATS, proc_table.all_pids, pid_array_index_offset);
    pid_index += config.NOF_WORKER_SEATS;
    assign_service_sportello(spor_msg_id, config.NOF_WORKER_SEATS);

    //Operatore
    create_processes("./operatore", config.NOF_WORKERS, proc_table.all_pids, pid_array_index_offset);
*/

    wait_children_ready(sem_id, proc_table.n_pids);     //direttore aspetta che i figli siano tutti pronti
    run_simulation(config.SIM_DURATION, config.N_NANO_SECS, sem_id, proc_table.n_pids);

    //Cleanup malloc
    free(proc_table.all_pids);
    printf("[DIRETTORE] ======== Fine simulazione ========\n");

    return 0;
}