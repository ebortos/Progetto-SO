#include "../include/shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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

//info simulazione
typedef struct {
    int giorno_corrente;
    int minuto_corrente;    //da capire se utile
    
    //info giornaliere (reset ogni giorno)
    int utenti_serviti_oggi;        //*forse inutile, perchè si potrebbe direttamente aggiornare 
    int servizi_erogati_oggi;       // la conta totale di clienti e servizi
    int servizi_non_erogati_oggi;
    double tempo_attesa_tot_oggi;   //idem con patate a quello sopra*
    int operatori_attivi_oggi;      //**in teoria neanche questo serve dato che il n di operatori è definito nel config
                                    
    // Statistiche totali
    int utenti_serviti_tot;
    int servizi_erogati_tot;
    int servizi_non_erogati_tot;
    double tempo_attesa_tot;
    int pause_tot;
    int operatori_attivi_tot;       //idem a sopra**
    
    // Controllo terminazione
    int utenti_in_attesa;
    
} dati_sim_t;

extern char **environ; //per execve

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

void create_erogatore_ticket() {
    char *argv[] = {"erogatore", "OK", NULL};
    pid_t  pid = fork();
    
    if (pid < 0) {
        perror("Errore fork erogatore_ticket.\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        if (execve("./erogatore", argv, environ) == -1) {
            perror("Errore execve erogatore_ticket.\n");
            exit(EXIT_FAILURE);
        }
    }

    waitpid(pid, NULL, 0); //DEBUG, per sincronizzare l'output
}

//PID = PORCO IL DIO

void create_sportello(config_t *config) {
    char *argv[] = {"sportello", "OK", NULL};
    
    for (int i = 0; i < config->NOF_WORKER_SEATS; i++) {
        pid_t  pid = fork();

        if (pid < 0) {
            perror("Errore fork sportello.\n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (execve("./sportello", argv, environ) == -1) {
                perror("Errore execve sportello.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 0; i < config->NOF_WORKER_SEATS; i++) { 
        wait(NULL); //DEBUG, per sincronizzare l'output
    }
}

void create_operatore(config_t *config) {
    char *argv[] = {"operatore", "OK", NULL};
    
    for (int i = 0; i < config->NOF_WORKERS; i++) {
        pid_t  pid = fork();

        if (pid < 0) {
            perror("Errore fork operatore.\n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (execve("./operatore", argv, environ) == -1) {
                perror("Errore execve operatore.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 0; i < config->NOF_WORKERS; i++) { 
        wait(NULL); //DEBUG, per sincronizzare l'output
    }
}

void create_utente(config_t *config) {
    char *argv[] = {"utente", "OK", NULL};
    
    for (int i = 0; i < config->NOF_USERS; i++) {
        pid_t  pid = fork();

        if (pid < 0) {
            perror("Errore fork utente.\n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (execve("./utente", argv, environ) == -1) {
                perror("Errore execve utente.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 0; i < config->NOF_USERS; i++) { 
        wait(NULL); //DEBUG, per sincronizzare l'output
    }
}

//**************************************************//
//per ora il main è principalmente usato per testare//
//**************************************************//
int main() {
    config_t config;

    if (load_config("../config.conf", &config) != 0) {
        perror("Errore nella lettura del file.\n");
        return -1;
    }
    
    //DEBUG
    //**************************************************//
    create_erogatore_ticket();
    create_sportello(&config);
    create_operatore(&config);
    create_utente(&config);
    //**************************************************//

    return 0;
}