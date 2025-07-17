#ifndef SHARED_H
#define SHARED_H

//tipologie servizi
typedef enum {
    PACCHI = 0,
    LETTERE = 1,
    BANCOPOSTA = 2,
    BOLLETTINI = 3,
    PRODOTTI_FINANZIARI = 4,
    OROLOGI = 5
} tipo_servizio_t;

//sportello
typedef struct {
    int tipo_servizio;     //servizio erogato
    int occupato;          //0 = libero, 1 = occupato
    int operatore_id;      //operatore assegnato
} sportello_t;

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
    
} stato_simulazione_t;

#endif