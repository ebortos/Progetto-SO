#ifndef EROGATORE_H
#define EROGATORE_H

void init_dispenser(int msg_queue_id);
void issue_ticket( int user_id, int service_id);
void process_requests(int ticket_queue_id);
void send_stats(int director_queue_id);

#endif