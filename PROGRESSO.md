# TO-DO, Dubbi, Chiarimenti

- Sperando che il mr amico stia ancora pagando gpt, provi a sfruttarlo per quanto possibile (ho letto che chatgpt 3o è ottimo per questi lavori):)
al massimo lo pago io, 20 euro per passare sto esame dimmerda sono anche pochi+si può usare anche per db

## Linee Guida

- (era un prank) english preferred
- (Forse, tendente al no) la cosa migliore è scrivere il direttore prima e, ogni volta che si raggiunge la execve di un altro file (o in qualsiasi altro modo) si interrompe il direttore e si continua con quel file
- Pensavo di fare un'unico shared.h in cui spiaccicare tutte le definizioni per semplicità
- Tutte le strutture e definizioni (funzioni varie, struct, enum, typedef...) si scrivono nel .h se vengono utilizzate da più file, altrimenti finiscono nel .c rispettivo (quindi se una funzione non viene utilizzata da più file _non_ si mette il suo prototipo nel .h)
- Semafori system V per regolare i processi
- Memoria condivisa per le statistiche
- Code di messaggi (le pipe mi fanno sboccare) per la comunicazione fra processi
- Se alla fine della giornata ci sono più utenti in attesa di EXPLODE_TRESHOLD, la simulation diddio termina
- funzione unica per la creazione dei processi (semplicità e modularità)

## TO-DO

- capire che stracazzo significa il paragrafo 5.6 della consegna, in particolare la parte sui conf (forse guardare la lezione di presentazione del progetto di schifanella può essere d'aiuto, ora non ho voglia), in caso cambiare (easy) lettura dei file in direttore __(si può lasciare anche per dopo)__
- (eventualmente) assegnare numeri più facili al nome degli utenti (magari salvando il pid di ciascuno durante la creazione in un array e chiamandoli con il proprio indice)
- __continuare sincronizzazione erogatore con la sim, cercare di trovare un pattern per la gestione dei semafori da utilizzare anche negli altri file__

## Cose da fixare/controllare

- EROGATORE, sincornizzazione sballata
- Quando l'utente riceve il ticket, il numero del ticket sembra quasi casuale (da 1 a 5, sperimentale). Ogni nuova prova dovrebbe sempre far partire i ticket da 1 __SOLUZIONE:__ il problema probabilmente sta nel fatto che durante il debug le msgqueue vecchie non venivano chiuse, si risolve chiudendole nel direttore alla fine della simulazione
- capire come memorizzare questi dati _"le statistiche precedenti suddivise per tipologia di servizio"_ __(si può lasciare anche per dopo)__

## Cose fatte E testate

- Lettura config
- Makefile
- Creazione ed esecuzione processi (tutti ok yippie!!!!)
- Abbozzo msg queue erogatore-utente (work in progress, vedere cose da fixare)
- Assegnazione casuale servizio-sportello dal direttore con msg queue
