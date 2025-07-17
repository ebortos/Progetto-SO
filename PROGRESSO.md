# TO-DO, Dubbi, Chiarimenti

- Sperando che il mr amico stia ancora pagando gpt, provi a sfruttarlo per quanto possibile :)

## Linee Guida
- Per quanto possibile usare l'italiano
- (Forse) la cosa migliore è scrivere il direttore prima e, ogni volta che si raggiunge la execve di un altro file (o in qualsiasi altro modo) si interrompe il direttore e si continua con quel file
- Pensavo di fare un'unico shared.h in cui spiaccicare tutte le definizioni per semplicità
- Tutte le strutture e definizioni (struct, enum, typedef...) si scrivono nel .h se vengono utilizzate da più file, altrimenti finiscono nel .c rispettivo
- Semafori per regolare i processi
- Memoria condivisa per le statistiche
- Code di messaggi(?)(le pipe mi fanno sboccare) per la comunicazione fra processi
- Per tutte le stats non presenti nella struct o con un commento di fianco, si possono direttamente calcolare in un certo modo senza il bisogno di memorizzarle da nessuna parte per risparmiare spazio

## TO-DO
- Direttore.c: lettura e parsing del file config,
- inizializzazione processi

## Cose da fixare/controllare
- Per ora la struct con le stats è nel shared.h, probabilemnte andrà spostato nel direttore.c dato che le varie statistiche verranno passate tramite mem. cond. e quindi gli altri file non avranno bisogno di accedervi direttamente
- Per le stats vedere i commenti di fianco al codice
- capire come memorizzare questi dati _"le statistiche precedenti suddivise per tipologia di servizio"_