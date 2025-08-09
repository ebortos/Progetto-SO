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

#define NUM_PROCESSI (2 + config.NOF_USERS /*+ config.NOF_WORKER_SEATS + config.NOF_WORKERS*/)

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
process_table_t proc_table; //forse si può spostare
config_t config;

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

int create_processes(const char *exec_path, int count, pid_t *pid_array, int start_index, char *const argv[]) {
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

/* Spawns N sportelli, argv[1] = sportello id (0..N-1) */
static int create_sportelli(int n, pid_t *pid_array, int start_index) {
    for (int i = 0; i < n; ++i) {
        char idbuf[16];
        snprintf(idbuf, sizeof idbuf, "%d", i);

        char *const argv_sportello[] = { (char *)"./sportello", idbuf, NULL };
        create_processes("./sportello", 1, pid_array, start_index + i, argv_sportello);
    }
    return 0;
}

void assign_service_sportello(int msg_id, int num_sportelli, int log_qid) {
    for (int i = 0; i < num_sportelli; i++) {
        sportello_msg_t msg;
        msg.mtype = 1000 + i;       //1000 arbitrario per differenziare i msg
        msg.sportello_info.service_type = rand() % 6;
        msg.sportello_info.occupato = 0;

        if (msgsnd(msg_id, &msg, sizeof(sportello_t), 0) == -1) {
            perror("msgsnd direttore to sportello");
            exit(EXIT_FAILURE);
        }

        //log_sendf(log_qid, "[Direttore] Assegnato servizio %d a sportello %d\n", msg.sportello_info.service_type, i);
    }
}

void wait_children_ready(int sem_id, int n_ready) {
    printf("[DIRETTORE]   Attendo %d processi ready …\n", n_ready);

    for (int i = 0; i < n_ready; ++i)         /* blocca finché ognuno non fa +1 */
        sv_sem_wait(sem_id, 3);

    printf("[DIRETTORE]   Tutti i figli sono ready\n");
}

static void sem_broadcast(int sem_id, int sem_num, int count) {
    for (int i = 0; i < count; ++i)
        sv_sem_signal(sem_id, sem_num);
}

static void build_day_plan(int day, int n_sportelli, int spor_msg_qid, day_plan_t *plan)
{
    // reset plan
    memset(plan->counts, 0, sizeof(plan->counts));
    //plan->day = day;      se si decide di aggiungerlo

    for (int i = 0; i < n_sportelli; ++i) {
        int service = rand() % NUM_SERVICES;  // your policy can change

        // bump availability
        plan->counts[service]++;

        // notify sportello i of its assignment for today
        sportello_msg_t msg;
        msg.mtype = 1000 + i;                 // each sportello listens on 1000+id
        msg.sportello_info.service_type = service;
        msg.sportello_info.occupato     = 0;
        msg.sportello_info.operatore_id = -1;

        if (msgsnd(spor_msg_qid, &msg, sizeof(sportello_t), 0) == -1) {
            perror("msgsnd direttore->sportello");
            exit(EXIT_FAILURE);
        }
    }
}

void run_simulation(int sim_duration, long n_nano_secs, int sem_id, int n_broadcast, int log_qid, int spor_msg_qid, int n_sportelli, day_plan_t *plan) {
    struct timespec day = {
        .tv_sec = n_nano_secs / 1000000000,
        .tv_nsec = n_nano_secs % 1000000000
    };

    for (int d = 1; d <= sim_duration; d++) {
        sv_sem_set(sem_id, 0, 0);  //reset sem0
        sv_sem_set(sem_id, 1, 0);  //reset sem1

        build_day_plan(d, n_sportelli, spor_msg_qid, plan);

        log_sendf(log_qid, "[DIRETTORE] ======== Inizio giorno %d ========\n", d);
        sem_broadcast(sem_id, 0, n_broadcast);    //segnale di inizio giornata, sem 0
        assign_service_sportello(spor_msg_qid, config.NOF_WORKER_SEATS, log_qid);

        nanosleep(&day, NULL);

        sem_broadcast(sem_id, 1, n_broadcast);    //segnale di fine giornata, sem 1
        log_sendf(log_qid, "[DIRETTORE] ======== Fine giorno %d ==========\n", d);
        //Salvare stats qui (credo)
    }

    /* fine simulazione per TUTTI */
    sv_sem_set(sem_id, 2, 0);
    sem_broadcast(sem_id, 2, n_broadcast);
    sem_broadcast(sem_id, 0, n_broadcast);   /* sveglia chiunque sia fermo su sem0 (di sicurezza) */
}


//main in costruzione
//a meno di commenti "DEBUG" lasciare così che dovrebbe andare
int main() {
    setvbuf(stdout, NULL, _IOLBF, 0);   /* stdout line-buffered */

    srand(time(NULL) ^ getpid());

    if (load_config("../config.conf", &config) != 0) {
        perror("Errore nella lettura del file.\n");
        return -1;
    }

    //Init array pid processi
    proc_table.n_pids = 0;
    proc_table.all_pids = malloc(sizeof(pid_t) * NUM_PROCESSI);

    if (!proc_table.all_pids) {
        perror("malloc all_pids");
        exit(EXIT_FAILURE);
    }

    //Init semaphores
    key_t sem_key = ftok(FTOK_PATH_SEM, SEM_KEY_ID);
    int sem_id = create_semaphore_set(sem_key, 4);
    
    //Init msg queue logger
    int log_qid = open_log_queue();

    //Fresh init msg queue erogatore
    key_t erog_key = get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    int erog_mq = init_msg_queue_fresh(erog_key);

    //Init msg queue sportello
    key_t spor_queue_key = get_queue_key(FTOK_PATH_SPOR, MSG_QUEUE_ID_SPOR);
    int spor_msg_qid = init_msg_queue(spor_queue_key);

    // service/done queues (so they exist before children)
    int serv_qid = init_msg_queue_fresh(get_queue_key(FTOK_PATH_SERV, MSG_QUEUE_ID_SERV));
    (void)serv_qid;
    int done_qid = init_msg_queue(get_queue_key(FTOK_PATH_DONE, MSG_QUEUE_ID_DONE));
    (void)done_qid;

    /* === day plan SHM (RW) === */
    int plan_shmid = shm_plan_create_or_get();     // implement in utils as: ftok + shmget(IPC_CREAT|0666, sizeof(day_plan_t))
    day_plan_t *plan = shm_plan_attach_ro(plan_shmid);
    memset(plan, 0, sizeof(*plan));               // clear once


    //======= SPAWN PROCESSES ========//

    //Logger
    char *const args_logger[] = {(char *)"./logger", NULL};
    create_processes("./logger", 1, proc_table.all_pids, proc_table.n_pids, args_logger);
    proc_table.n_pids += 1;

    //Erogatore
    char *const args_erogatore[] = {(char *)"./erogatore", NULL};
    create_processes("./erogatore", 1, proc_table.all_pids, proc_table.n_pids, args_erogatore);
    proc_table.n_pids += 1;

    //Utente
    char p_serv_min[16], p_serv_max[16];
    snprintf(p_serv_min, sizeof(p_serv_min), "%f", config.P_SERV_MIN);
    snprintf(p_serv_max, sizeof(p_serv_max), "%f", config.P_SERV_MAX);
    char *const args_utente[] = {(char *)"./utente", p_serv_min, p_serv_max, NULL};
    create_processes("./utente", config.NOF_USERS, proc_table.all_pids, proc_table.n_pids, args_utente);
    proc_table.n_pids += config.NOF_USERS;

    //Sportello
    create_sportelli(config.NOF_WORKER_SEATS, proc_table.all_pids, proc_table.n_pids);
    proc_table.n_pids += config.NOF_WORKER_SEATS;
/*
    //Operatore
    create_processes("./operatore", config.NOF_WORKERS, proc_table.all_pids, pid_array_index_offset);
    // pid_array_index_offset chi è costui? un tipo losco
*/

    wait_children_ready(sem_id, proc_table.n_pids);     //direttore aspetta che i figli siano tutti pronti
    run_simulation(config.SIM_DURATION, config.N_NANO_SECS, sem_id, (proc_table.n_pids - 1), log_qid, spor_msg_qid, config.NOF_WORKER_SEATS, plan);

    //wait children (except logger -> i = 1)
    for (size_t i = 1; i < proc_table.n_pids; ++i) {
        waitpid(proc_table.all_pids[i], NULL, 0);
    }

    //close logger
    log_sendf(log_qid, "[DIRETTORE] ======== Fine simulazione ========\n");
    log_send_shutdown(log_qid);
    waitpid(proc_table.all_pids[0], NULL, 0);

    //cleanup
    shm_plan_detach(plan);

    cleanup_all_ipc();
    free(proc_table.all_pids);

    return 0;
}