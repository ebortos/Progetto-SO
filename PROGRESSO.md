# TO-DO, Dubbi, Chiarimenti

## FUNZIONA DIOMERDA

## Linee Guida

- sparsi nel codice ci sono delle printf (log_send) commentate, di base non servono ma possono essere utili per testing
- (era un prank) english preferred nel codice, italiano nelle stampe
- Pensavo di fare un'unico shared.h in cui spiaccicare tutte le definizioni per semplicità
- Tutte le strutture e definizioni (funzioni varie, struct, enum, typedef...) si scrivono nel .h se vengono utilizzate da più file, altrimenti finiscono nel .c rispettivo (quindi se una funzione non viene utilizzata da più file _non_ si mette il suo prototipo nel .h)
- Semafori system V per regolare i processi
- Memoria condivisa per le statistiche
- Code di messaggi (le pipe mi fanno sboccare) per la comunicazione fra processi
- Se alla fine della giornata ci sono più utenti in attesa di EXPLODE_TRESHOLD, la simulation diddio termina
- funzione unica per la creazione dei processi (semplicità e modularità)
-__SEMAFORI:__0 = inizio giornata, 1 = fine giornata, 2 = fine sim, 3 = ready-barrier per i figli (dir aspetta che siano tutti inizializzati prima di proseguire con il primo giorno)
- rinominate le funzioni utils dei sem a sv_sem_.... per evitare ambiguità con le funzioni posix (in particolare per sem_wait)
- aggiunto logger.c: tutti i processi per stampare mandano tramite un msg queue al logger il messaggio che poi li stampa in ordine
- aggiunta shared memory tra direttore e utente per comunicare gli sportelli disponibili del giorno con i relativi servizi
- l'utente, dopo essersi trastullato allo sportello viene rimosso dalla coda "implicitamente" con la chiamata __ssize_t r = msgrcv(serv_qid, &req, sizeof(req) - sizeof(long), (long)(my_service + 1), IPC_NOWAIT);__ eliminando di fatto la richiesta dell'utente dalla msgq
- ora il direttore aspetta un segnale ready (sem3) alla fine di ogni giorno per assicurarsi che tutti i figli abbiano finito e siano pronti per il giorno successivo
- operatore ha il 20% di possibilità di andare in pausa per 10 min

## TO-DO

- Aggiungere statistiche con shm
- aggiungere controllo explode threshold
- capire che stracazzo significa il paragrafo 5.6 della consegna, in particolare la parte sui conf (forse guardare la lezione di presentazione del progetto di schifanella può essere d'aiuto, ora non ho voglia), in caso cambiare (easy) lettura dei file in direttore __(si può lasciare anche per dopo)__
- __ALLA FINE:__ guardare utils.c e rimuovere tutte le funzioni non utilizzate
- cambiare tutte le chiamate di funzione per creare ipc con una funzione dedicata (es: vedere open_log_queue())__(si può lasciare anche per dopo)__

## Cose da fixare/controllare

- logger non stampa shutdown finale (da controllare se sia effettivamente un problema)
- capire come memorizzare questi dati _"le statistiche precedenti suddivise per tipologia di servizio"_ __(si può lasciare anche per dopo)__
- capire se il controllo sem4 ha senso durante la giornata (after wake), __(si può lasciare anche per dopo)__

## Statistiche

- il numero di utenti serviti totali nella simulazione
- il numero di utenti serviti in media al giorno
- il numero di servizi erogati totali nella simulazione
- il numero di servizi non erogati totali nella simulazione
- il numero di servizi erogati in media al giorno
- il numero di servizi non erogati in media al giorno
- il tempo medio di attesa degli utenti nella simulazione
- il tempo medio di attesa degli utenti nella giornata
- il tempo medio di erogazione dei servizi nella simulazione
- il tempo medio di erogazione dei servizi nella giornata
- le statistiche precedenti suddivise per tipologia di servizio
- il numero di operatori attivi durante la giornata;
- il numero di operatori attivi durante la simulazione;
- il numero medio di pause effettuate nella giornata e il totale di pause effettuate durante la simulazione;
- il rapporto fra operatori disponibili e sportelli esistenti, per ogni sportello per ogni giornata.

## Cose fatte E testate

- Lettura config
- Makefile
- Creazione ed esecuzione processi (tutti ok yippie!!!!)
- Msg queue erogatore-utente
- Assegnazione casuale servizio-sportello dal direttore con msg queue
- Semafori erogatore
- Semafori utente
- Logger
- Pulizia ipcs
- Utente ora può chiedere più ticket durante la sim
- Semafori sportello
- Utente si mette in coda allo sportello e viene poi tolto
- Utente viene servito allo sportello
- Utente viene interrotto alla fine della giornata e della sim se in mezzo a un servizio
- Operatore va in pausa lasciando lo sportello libero per altri
