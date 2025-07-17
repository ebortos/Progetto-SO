#include "../include/ipc_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define _GNU_SOURCE
#define MAX_SERVICES 6
#define N_REQUESTS_MAX 5
#define P_SERV_MIN 0.2
#define P_SERV_MAX 0.8

typedef struct {
    int user_id;
    int service_id;
    time_t request_time;
} ServiceRequest;

typedef struct {
    float avg_daily_time;
    float avg_total_time;
} Stats;

static int total_wait_time = 0;
static int total_requests = 0;
static int daily_wait_time = 0;
static int daily_requests = 0;


void wait_for_service(int msg_queue_id, int user_id, time_t request_time) {
    ServiceRequest response;

    if (msgrcv(msg_queue_id, &response, sizeof(ServiceRequest) - sizeof(long), user_id, 0) == -1) {
        perror("Error receiving service completion");
        exit(EXIT_FAILURE);
    }

    time_t completion_time = time(NULL);
    int wait_time = (int)difftime(completion_time, request_time);

    total_wait_time += wait_time;
    total_requests++;
    daily_wait_time += wait_time;
    daily_requests++;

    printf("User %d: completed service %d. Waiting time: %d seconds.\n", response.user_id, response.service_id, wait_time);
}

void user_process(int user_id, int msg_queue_id, int director_queue_id, float p_serv, int n_nano_secs) {
    srand(getpid() + time(NULL));

    while (1) {
        float decision = (float)rand() / RAND_MAX;

        if (decision > p_serv) {
            printf("Today user %d is not going to the post office.\n", user_id);
            sleep(1);
            continue;
        }

        daily_wait_time = 0;
        daily_requests = 0;

        int delay_time = rand() % n_nano_secs;
        usleep(delay_time * 1000);
        printf("User %d decided to go to the post office after %d minutes.\n", user_id, delay_time);

        int n_request = rand() % N_REQUESTS_MAX + 1;

        int *service_list = malloc(n_request * sizeof(int));
        if (service_list == NULL) {
            perror("Error allocating memory for service_list");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < n_request; i++) {
            service_list[i] = rand() % MAX_SERVICES;
        }

        printf("User %d goes to the post office with %d requests.\n", user_id, n_request);

        for (int i = 0; i < n_request; i++) {
            ServiceRequest request;
            request.user_id = user_id;
            request.service_id = service_list[i];
            request.request_time = time(NULL);

            if ((request.request_time % n_nano_secs) + delay_time >= n_nano_secs) {
                printf("User %d: Not enough time to complete service %d today. Going home.\n", user_id, service_list[i]);
                break;
            }

            if (msgsnd(msg_queue_id, &request, sizeof(ServiceRequest) - sizeof(long), 0) == -1) {
                perror("Error sending user request");
                exit(EXIT_FAILURE);
            }

            printf("User %d sent request for service %d.\n", user_id, service_list[i]);
            wait_for_service(msg_queue_id, user_id, request.request_time); 
        }

        if (daily_requests > 0) {
            Stats stats;
            stats.avg_daily_time = (float)daily_wait_time / daily_requests;
            stats.avg_total_time = (float)total_wait_time / total_requests;

            if (msgsnd(director_queue_id, &stats, sizeof(Stats)- sizeof(long), 0) == -1) {
                perror("Error sending stats to director.\n");
                exit(EXIT_FAILURE);
            }
        }

        free(service_list);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        perror("Error number of arg ./user.\n");
        exit(EXIT_FAILURE);
    }

    key_t msg_key = atoi(argv[1]);
    key_t director_key = atoi(argv[2]);
    int user_id = atoi(argv[3]);
    int n_nano_secs = atoi(argv[4]);

    int msg_queue_id = init_message_queue(msg_key);
    int director_queue_id = init_message_queue(director_key);

    float p_serv = P_SERV_MIN + (float)rand() / RAND_MAX * (P_SERV_MAX - P_SERV_MIN);
    printf("User %d: probability of going to the post office %.2f.\n", user_id, p_serv);

    user_process(user_id, msg_queue_id, director_queue_id, p_serv, n_nano_secs);

    return 0;
}