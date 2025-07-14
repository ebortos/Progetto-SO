#include "../include/ipc_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>

#define _GNU_SOURCE
#define MAX_SERVICES 6
#define TICKET_BASE 1000

typedef struct {
    long mtype;
    int user_id;
    int service_id;
} Request;

typedef struct {
    long mtype;
    int user_id;
    int ticket_id;
    int service_id;
} Ticket;

static struct {
    int ticket_counter;
    int msg_queue_id;
} dispenser;

typedef struct {
    long mtype;
    int service_id;
    int total_tickets;
} Stats;

static int ticket_count[MAX_SERVICES] = {0};

void init_dispenser(int msg_queue_id) {
    dispenser.ticket_counter = TICKET_BASE;
    dispenser.msg_queue_id = msg_queue_id;

    printf(">> Ticket dispenser started with msg queue %d. Starting ticket %d.\n", msg_queue_id, TICKET_BASE);
}

void issue_ticket( int user_id, int service_id) {
    if (service_id < 0 || service_id >= MAX_SERVICES) {
        fprintf(stderr, "Error invalid service (%d).\n", service_id);
        return;
    }

    Ticket ticket;
    ticket.mtype = 1;
    ticket.user_id = user_id;
    ticket.ticket_id = dispenser.ticket_counter++;
    ticket.service_id = service_id;

    if (msgsnd(dispenser.msg_queue_id, &ticket, sizeof(Ticket) - sizeof(long), 0) == -1) {
        perror("Error sending ticket.\n");
        exit(EXIT_FAILURE);
    }

    ticket_count[service_id]++;

    printf("Issued ticket %d for user %d, service id %d.\n", ticket.ticket_id, user_id, service_id);
}

void process_requests(int ticket_queue_id) {
    Request request;

    while (1) {
        printf("Waiting for user requests...\n");

        if (msgrcv(ticket_queue_id, &request, sizeof(request) - sizeof(long), 0, 0) == -1) {
            perror("Error receiving user request.");
            exit(EXIT_FAILURE);
        }

        printf("Received request from user %d for service %d.\n", request.user_id, request.service_id);

        issue_ticket(request.user_id, request.service_id);
    }
}

void send_stats(int director_queue_id) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        Stats stats;
        stats.mtype = 1;
        stats.service_id = i;
        stats.total_tickets = ticket_count[i];

        if (msgsnd(director_queue_id, &stats, sizeof(Stats) - sizeof(long), 0) == -1) {
            perror("Error sending statistics to director.");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        perror("Error number of arguments ./erogatore.\n");
        exit(EXIT_FAILURE);
    }

    key_t msg_key = atoi(argv[1]);
    key_t director_key = atoi(argv[2]);
    int ticket_queue_id = init_message_queue(msg_key);
    int director_queue_id = init_message_queue(director_key);

    printf (">> Ticket dispenser process started.\n");

    init_dispenser(ticket_queue_id);

    process_requests(ticket_queue_id);
    
    send_stats(director_queue_id);

    destroy_message_queue(ticket_queue_id);
    destroy_message_queue(director_queue_id);

    printf(">> Ticket dispenser process completed.\n");
    

    return 0;
}