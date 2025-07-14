#ifndef DIRETTORE_H
#define DIRETTORE_H

#include <stdio.h>

typedef struct Config;
typedef struct DailyStats;
typedef struct Operatore;

void read_config(Config *config);
void log_stats_to_csv(FILE *csv_file, int day, DailyStats *stats);
void assign_services_to_counters(int *counters, int nof_workers);
void assign_service_to_operators(Operatore *operatori, int nof_workers);
void notify_operators(int nof_workers);
void handle_termination(int cause, int total_days, float total_avg_wait_time, int total_users_served, int total_users_left_waiting);

#endif