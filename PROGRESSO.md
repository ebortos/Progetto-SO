# TO-DO, Dubbi, Chiarimenti

## Linee Guida

- (era un prank) english preferred nel codice, italiano nelle stampe
- (Forse, tendente al no) la cosa migliore è scrivere il direttore prima e, ogni volta che si raggiunge la execve di un altro file (o in qualsiasi altro modo) si interrompe il direttore e si continua con quel file
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

## TO-DO

- continuare operatore
- capire che stracazzo significa il paragrafo 5.6 della consegna, in particolare la parte sui conf (forse guardare la lezione di presentazione del progetto di schifanella può essere d'aiuto, ora non ho voglia), in caso cambiare (easy) lettura dei file in direttore __(si può lasciare anche per dopo)__
- (eventualmente) assegnare numeri più facili al nome degli utenti (magari salvando il pid di ciascuno durante la creazione in un array e chiamandoli con il proprio indice)
- __ALLA FINE:__ guardare utils.c e rimuovere tutte le funzioni non utilizzate
- cambiare tutte le chiamate di funzione per creare ipc con una funzione dedicata (es: vedere open_log_queue())__(si può lasciare anche per dopo)__

## Cose da fixare/controllare

- logger non stampa shutdown finale
- capire come memorizzare questi dati _"le statistiche precedenti suddivise per tipologia di servizio"_ __(si può lasciare anche per dopo)__
- capire se il controllo sem4 ha senso durante la giornata (after wake), __(si può lasciare anche per dopo)__

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
- Utente si mette in coda allo sportello
