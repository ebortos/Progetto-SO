#ifndef OPERATORE_H
#define OPERATORE_H

void handle_signal(int sig);
void send_stats(int msg_queue_id);
int calc_service_time(int base_time);
void process_ticket(int msg_queue_id, int operator_service);

#endif