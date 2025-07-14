#include "../include/ipc_utils.h"
#include "../include/sportello.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>

#define _GNU_SOURCE
#define NOF_PAUSE 3
#define MAX_SERVICES 6
#define PAUSE_PROBABILITY 20    // Percentuale di probabilità di pausa
#define PAUSE_DURATION 5        // Durata della pausa in secondi
#define VARIANCE_PERCENTAGE 50 // Percentuale massima di variazione nei tempi di servizio

typedef struct {
    int user_id;
    int service_id;
    time_t request_time;
} ServiceRequest;

typedef struct {
    int service_id;
    int user_id;
    time_t response_time;
} ServiceResponse;

typedef struct {
    int user_id;
    int ticket_id;
    int service_id;
} Ticket;

typedef struct {
    int total_tickets;
    int total_time;
} Stats;

static Stats stats[MAX_SERVICES];
static int running = 1, n_pause = 0;
static int current_sportello = -1;
static const int service_times[MAX_SERVICES] = {10, 15, 20, 25, 30, 35};
static int can_start_searching = 0;

void handle_signal(int sig) {
    if (sig == SIGINT) {
        running = 0;
    } else if (sig == SIGUSR1) {
        can_start_searching = 1;
    }
}

void send_stats(int msg_queue_id) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        Ticket stat_message;
        stat_message.user_id = -1;
        stat_message.ticket_id = stats[i].total_tickets;
        stat_message.service_id = stats[i].total_time;

        if (msgsnd(msg_queue_id, &stat_message, sizeof(Ticket) - sizeof(long), 0) == -1) {
            perror("Error sending stats");
            exit(EXIT_FAILURE);
        }
    }
}

int calc_service_time(int base_time) {
    int variance = (base_time * VARIANCE_PERCENTAGE) / 100;
    int min_time = base_time - variance;
    int max_time = base_time + variance;

    return min_time + (rand() % (max_time - min_time + 1));
}

void process_ticket(int msg_queue_id, int operator_service) {
    ServiceRequest request;

    if (msgrcv(msg_queue_id, &request, sizeof(Ticket) - sizeof(long), 0, 0) == -1) {
        perror("Error receiving ticket");
        exit(EXIT_FAILURE);
    }

    if (request.service_id != operator_service) {
        fprintf(stderr, "Ticket received for incompatible service. Skipping.\n");
        return;
    }

    int processing_time = calc_service_time(service_times[request.service_id]);

    printf("Processing request from user %d for service %d. Time: %d seconds.\n", request.user_id, request.service_id, processing_time);
    sleep(processing_time);

    stats[request.service_id].total_tickets++;
    stats[request.service_id].total_time += processing_time;

    ServiceResponse response;
    response.user_id = request.user_id;
    response.service_id = request.service_id;
    response.response_time = time(NULL);

    if (msgsnd(msg_queue_id, &response, sizeof(ServiceResponse) - sizeof(long), 0) == -1) {
        perror("Error sending service response");
        exit(EXIT_FAILURE);
    }

    printf("Finished processing request for user %d, service %d.\n", request.user_id, request.service_id);

    if ((rand() % 100) < PAUSE_PROBABILITY && n_pause < NOF_PAUSE) {
        printf("Operator taking a pause for %d seconds.\n", PAUSE_DURATION);

        libera_sportello(current_sportello);
        current_sportello = -1;

        sleep(PAUSE_DURATION);
        n_pause++;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        perror("Error number of arguments ./operator.\n");
        exit(EXIT_FAILURE);
    }

    key_t msg_key = atoi(argv[1]);
    key_t director_key = atoi(argv[2]);
    int operator_service = atoi(argv[3]);
    key_t shm_key = atoi(argv[4]);
    key_t sem_key = atoi(argv[5]);

    int sem_id = semget(sem_key, 0, 0666);
    if (sem_id == -1) {
        perror("Errore accesso ai semafori");
        exit(EXIT_FAILURE);
    }

    int msg_queue_id = init_message_queue(msg_key);
    int director_queue_id = init_message_queue(director_key);

    for (int i = 0; i < MAX_SERVICES; i++) {
        stats[i].total_tickets = 0;
        stats[i].total_time = 0;
    }

    srand(getpid());
    signal(SIGINT, handle_signal);
    signal(SIGUSR1, handle_signal);

    printf(">> Operator process started.\n");

    int shm_id = create_shared_memory(shm_key, sizeof(Sportello));
    Sportello *sportelli = attach_shared_memory(shm_id);
    
    while (!can_start_searching) {
        sleep(1);
    }
    printf("debug2\n");
    current_sportello = cerca_sportello_libero(sportelli, 2, operator_service, 0);
    printf("debug3\n");
    if (current_sportello == -1) {
        printf("No office available. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    printf(">> Operator assigned to office %d. Waiting for tickets...\n", current_sportello);

    process_ticket(msg_queue_id, operator_service);

    printf(">> Operator process shutting down...\n");
    send_stats(director_queue_id);

    detach_shared_memory(sportelli);
    return 0;
}
