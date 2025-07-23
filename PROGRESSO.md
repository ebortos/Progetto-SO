# TO-DO, Dubbi, Chiarimenti

- Sperando che il mr amico stia ancora pagando gpt, provi a sfruttarlo per quanto possibile :)

## Linee Guida
- Per quanto possibile usare l'italiano
- (Forse, tendente al no) la cosa migliore è scrivere il direttore prima e, ogni volta che si raggiunge la execve di un altro file (o in qualsiasi altro modo) si interrompe il direttore e si continua con quel file
- Pensavo di fare un'unico shared.h in cui spiaccicare tutte le definizioni per semplicità
- Tutte le strutture e definizioni (funzioni varie, struct, enum, typedef...) si scrivono nel .h se vengono utilizzate da più file, altrimenti finiscono nel .c rispettivo (quindi se una funzione non viene utilizzata da più file _non_ si mette il suo prototipo nel .h)
- Semafori per regolare i processi
- Memoria condivisa per le statistiche
- Code di messaggi(?)(le pipe mi fanno sboccare) per la comunicazione fra processi
- Per tutte le stats non presenti nella struct o con un commento di fianco, si possono direttamente calcolare in un certo modo senza il bisogno di memorizzarle da nessuna parte per risparmiare spazio
- Per la __simulazione__, dir manda segnali (sigusr1 in teoria), agli altri processi. Poi si chiama nanosleep(_con gli argomenti relativi_) alla cui fine si mandano segnali (sigusr2 credo) ai processi di terminare.
- Se alla fine della giornata ci sono più utenti in attesa di EXPLODE_TRESHOLD, la simulation diddio termina

## TO-DO
- capire che stracazzo significa il paragrafo 5.6 della consegna, in particolare la parte sui conf (forse guardare la lezione di presentazione del progetto di schifanella può essere d'aiuto, ora non ho voglia), in caso cambiare (easy) lettura dei file in direttore __(si può lasciare anche per dopo)__
- scrivere codice erogatore
- scrivere codice sportello
- scrivere codice operatore
- scrivere codice utente
- scrivere pezzo simulazione (probabilmente conviene fare una funzione a parte)

## Cose da fixare/controllare
- Per le stats vedere i commenti di fianco al codice
- capire come memorizzare questi dati _"le statistiche precedenti suddivise per tipologia di servizio"_ __(si può lasciare anche per dopo)__
- capire se usare semafori posix o system v __(si può lasciare anche per dopo, non troppo dopo però)__

## Cose fatte E testate
- Lettura config
- Makefile
- Creazione ed esecuzione processi (tutti ok yippie!!!!)
