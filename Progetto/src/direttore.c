#define _GNU_SOURCE

#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#define NUM_PROCESSI (2 + config.NOF_USERS + config.NOF_WORKER_SEATS + config.NOF_WORKERS)

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

static void create_operatori() {
    char nsbuf[32];
    snprintf(nsbuf, sizeof nsbuf, "%d", config.N_NANO_SECS);

    char pausesbuf[16];
    snprintf(pausesbuf, sizeof pausesbuf, "%d", config.NOF_PAUSE);

    for (int i = 0; i < config.NOF_WORKERS; ++i) {
        int svc = rand() % NUM_SERVICES;

        char svcbuf[16];
        snprintf(svcbuf, sizeof svcbuf, "%d", svc);

        char *const args_op[] = { (char *)"./operatore", svcbuf, nsbuf, pausesbuf, NULL };
        create_processes("./operatore", 1, proc_table.all_pids, proc_table.n_pids + i, args_op);
    }
}

void wait_children_ready(int sem_id, int n_ready) {
    //printf("[DIRETTORE]   Attendo %d processi ready …\n", n_ready);

    for (int i = 0; i < n_ready; ++i)         /* blocca finché ognuno non fa +1 */
        sv_sem_wait(sem_id, 3);

    //printf("[DIRETTORE]   Tutti i figli sono ready\n");
}

static void sem_broadcast(int sem_id, int sem_num, int count) {
    for (int i = 0; i < count; ++i)
        sv_sem_signal(sem_id, sem_num);
}

static void assign_service_sportello(int day, int n_sportelli, int spor_msg_qid, day_plan_t *plan) {
    memset(plan->counts, 0, sizeof(plan->counts));
    

    for (int i = 0; i < n_sportelli; ++i) {
        int service = rand() % NUM_SERVICES;
        plan->counts[service]++;

        sportello_msg_t msg;
        msg.mtype = 1000 + i;                     // sportello i listens here
        msg.sportello_info.service_type = service;
        msg.sportello_info.occupato     = 0;
        msg.sportello_info.operatore_id = -1;

        /* IMPORTANT: size is payload excluding long mtype */
        if (msgsnd(spor_msg_qid, &msg, MSGSZ(sportello_msg_t), 0) == -1) {
            perror("msgsnd direttore->sportello");
            exit(EXIT_FAILURE);
        }
    }
}

static int check_explode(int sem_id, int n_broadcast, int stats_qid, int explode_threshold, int log_qid) {
    int interrupted_today = 0;
    int per_service[NUM_SERVICES] = {0};

    erogatore_request_msg sm;
    while (1) {
        ssize_t r = msgrcv(stats_qid, &sm, MSGSZ(erogatore_request_msg), 0, IPC_NOWAIT | MSG_NOERROR);
        if (r == -1) {
            if (errno == ENOMSG) break;
            perror("msgrcv (stats)"); exit(EXIT_FAILURE);
        }
        interrupted_today++;
        if (sm.service_type >= 0 && sm.service_type < NUM_SERVICES)
            per_service[sm.service_type]++;
    }

    log_sendf(log_qid, "[DIRETTORE] Interrotti oggi: %d (soglia=%d)\n", interrupted_today, explode_threshold);
    for (int s = 0; s < NUM_SERVICES; ++s)
        if (per_service[s] > 0)
            log_sendf(log_qid, "  - servizio %d: %d\n", s, per_service[s]);

    if (interrupted_today > explode_threshold) {
        log_sendf(log_qid, "[DIRETTORE] *** EXPLODE TRESHOLD: TERMINO LA SIMULAZIONE ***\n");
        sv_sem_set(sem_id, 2, 0);
        sem_broadcast(sem_id, 2, n_broadcast);
        return 1;
    }
    return 0;
}

void run_simulation(int sim_duration, const long NANOS_SIM_MIN, int sem_id, int n_broadcast, int log_qid, int spor_msg_qid, int n_sportelli, day_plan_t *plan, int explode_qid) {
    const int DAY_SIM_MINUTES = 8 * 60;  // 8h

    struct timespec day = {
        .tv_sec  = (NANOS_SIM_MIN * (long)DAY_SIM_MINUTES) / 1000000000L,
        .tv_nsec = (NANOS_SIM_MIN * (long)DAY_SIM_MINUTES) % 1000000000L
    };

    key_t seats_key = ftok(FTOK_PATH_SEATS, SEM_SEATS_ID);
    int seats_sid = semget(seats_key, NUM_SERVICES, 0);
    if (seats_sid == -1) { perror("semget seats"); exit(EXIT_FAILURE); }

    for (int d = 1; d <= sim_duration; d++) {
        sv_sem_set(sem_id, 0, 0);  //reset sem0
        sv_sem_set(sem_id, 1, 0);  //reset sem1
        sv_sem_set(sem_id, 3, 0);  //reset sem3

        purge_queue_all(spor_msg_qid);
        assign_service_sportello(d, n_sportelli, spor_msg_qid, plan);

        for (int s = 0; s < NUM_SERVICES; ++s)  //pubblicazione posti sportello
            sv_sem_set(seats_sid, (unsigned short)s, plan->counts[s]);

        log_sendf(log_qid, "\n[DIRETTORE] ======== Inizio giorno %d ========\n\n", d);
        sem_broadcast(sem_id, 0, n_broadcast);    //segnale di inizio giornata, sem 0

        nanosleep(&day, NULL);

        sem_broadcast(sem_id, 1, n_broadcast);    //segnale di fine giornata, sem 1
        wait_children_ready(sem_id, n_broadcast);

        if (check_explode(sem_id, n_broadcast, explode_qid, config.EXPLODE_THRESHOLD, log_qid)) break;

        log_sendf(log_qid, "\n[DIRETTORE] ======== Fine giorno %d ==========\n", d);
        //Salvare stats qui (credo)
    }

    /* fine simulazione per TUTTI */
    sv_sem_set(sem_id, 2, 0);
    sem_broadcast(sem_id, 2, n_broadcast);
    sem_broadcast(sem_id, 0, n_broadcast);   /* sveglia chiunque sia fermo su sem0 (di sicurezza) */
}

int main() {
    setvbuf(stdout, NULL, _IOLBF, 0);   /* stdout line-buffered */

    srand(time(NULL) ^ getpid());

    if (load_config("../config.conf", &config) != 0) {
        perror("Errore nella lettura del file.\n");
        return -1;
    }


    // ======= SPAWN IPCS AND VARS ======= //

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

    //Init sem seats
    key_t seats_key = ftok(FTOK_PATH_SEATS, SEM_SEATS_ID);
    int seats_sid = create_semaphore_set(seats_key, NUM_SERVICES);
    
    //Init msg queue logger
    int log_qid = open_log_queue();

    //Fresh init msg queue erogatore
    key_t erog_key = get_queue_key(FTOK_PATH_EROG, MSG_QUEUE_ID_EROG);
    int erog_mq = init_msg_queue_fresh(erog_key);

    //Init msg queue sportello
    key_t spor_queue_key = get_queue_key(FTOK_PATH_SPOR, MSG_QUEUE_ID_SPOR);
    int spor_msg_qid = init_msg_queue(spor_queue_key);

    // service/done queues
    int serv_qid = init_msg_queue_fresh(get_queue_key(FTOK_PATH_SERV, MSG_QUEUE_ID_SERV));
    //(void)serv_qid;
    int done_qid = init_msg_queue(get_queue_key(FTOK_PATH_DONE, MSG_QUEUE_ID_DONE));
    (void)done_qid;

    //Init explode queue fresh
    int explode_qid = init_msg_queue_fresh(get_queue_key(FTOK_PATH_EXPLODDE, MSG_QUEUE_ID_EXPLODE));


    //day plan (read write)
    key_t plan_key = ftok(FTOK_PATH_PLAN, SHM_PLAN_ID);
    int   plan_shmid = shmget(plan_key, sizeof(day_plan_t), IPC_CREAT | 0666);
    if (plan_shmid == -1) { perror("shmget plan"); exit(EXIT_FAILURE); }
    day_plan_t *plan = shm_attach(plan_shmid, 0);
    if (plan == (void *)-1) { perror("shmat plan"); exit(EXIT_FAILURE); }
    memset(plan, 0, sizeof(*plan));              // clear once


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

    //Operatore
    create_operatori();
    proc_table.n_pids += config.NOF_WORKERS;


    // ======= SIMULATION ======= //

    wait_children_ready(sem_id, proc_table.n_pids);     //direttore aspetta che i figli siano tutti pronti
    run_simulation(config.SIM_DURATION, config.N_NANO_SECS, sem_id, (proc_table.n_pids - 1), log_qid, spor_msg_qid, config.NOF_WORKER_SEATS, plan, explode_qid);

    //wait children (except logger -> i = 1)
    for (size_t i = 1; i < proc_table.n_pids; ++i) {
        waitpid(proc_table.all_pids[i], NULL, 0);
    }

    //close logger
    log_sendf(log_qid, "\n[DIRETTORE] ======== Fine simulazione ========\n");
    log_send_shutdown(log_qid);
    waitpid(proc_table.all_pids[0], NULL, 0);

    //cleanup
    shmdt(plan);
    cleanup_all_ipc();
    free(proc_table.all_pids);

    return 0;
}