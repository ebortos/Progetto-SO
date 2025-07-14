#ifndef UTENTE_H
#define UTENTE_H

#include <sys/time.h>

void wait_for_service(int msg_queue_id, int user_id, time_t request_time);
void user_process(int user_id, int msg_queue_id, int director_queue_id, float p_serv, int n_nano_secs);


#endif