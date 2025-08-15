
1. Protocolul de trimitere a mesajelor
Mesajele sunt trimise folosind o structura simpla, definită astfel:

struct __attribute__((packed)) tcp_message{
    char id[11];
    char command[15];
    char message[51];
};

De ce am folosit aceasta structura?

- Alinierea optimizata: Folosind __attribute__((packed)), evitam spatiile goale intre campurile structurii, asigurand ca mesajele 
sunt trimise exact asa cum sunt in memorie.
- ID-ul clientului: Este esential pentru a identifica fiecare client in mod unic.
- Comanda: Aceasta indica ce trebuie sa faca serverul, - sa se aboneze sau sa dezaboneze un client.
- Mesajul: Topicul pe care se aplica comanda.


2. De ce am folosit epoll si nu poll?
Am ales epoll in loc de poll dintr-un motiv simplu: epoll este mult mai eficient atunci cand avem multe conexiuni deschise simultan. 
Poll are o complexitate de O(n), ceea ce inseamna ca trebuie sa verificam fiecare conexiune in parte la fiecare iteratie. In schimb, 
epoll are o complexitate O(1) si este mult mai eficient atunci cand numarul de conexiuni creste.


3. Implementare map + Functionalitate

Am implementat un map key-value care sa ma ajute la urmatoarele taskuri:

    - Maparea topicurilor se face folosind o structura de date de tip map, care stocheaza perechi cheie-valoare. 
    Cheia este id-ul clientuli, iar valoarea este un array de stringuri care contin numele topicurilor la care este abonat clientul.

    - De asemenea, unicitatea id-urilor este asigurata tot printr-un map. Diferenta este la functia apelata pentru
    inserarea in structura. Map-ul pentru unicitate are valori key-value string-int unde cheia este id-ul unic si valoarea este socketul pe
    care comunica serverul cu clientul respectiv.

4. Flow-ul programului

    1. Pornirea serverului
    Serverul deschide doua socketuri: 
    - Un socket TCP, pentru conexiunile cu clientii.
    - Un socket UDP, pentru a primi mesaje cu notificari.
    - Se foloseste epoll pentru a gestiona toate conexiunile active intr-un mod eficient. 
    - Adaugam socketul TCP, UDP si STDIN (pt comanda exit) in epoll pentru a monitoriza activitatea lor.

    2. Conectarea unui client
    - Cand un client se conecteaza prin TCP, serverul accepta conexiunea si citeste un id unic.
    (Pe partea de cod din subscriber, acesta vede ca este respins si isi inchide si el socketul pentru cleanup)
    - Daca exista deja un client cu acel id, conexiunea noua este respinsa.
    - Daca nu exista, clientul este adaugat in epoll si in map-ul care tine evidenta clientilor activi.

    3. Abonarea la topicuri
    - La "subscribe": Se adauga topicul in lista de topicuri asociati acelui client din map.
    - La "unsubscribe": Se elimina topicul din lista de mai sus.
    - La "exit", se elimina clientul din epoll si din mapa cu clientii conectati (cea id-socket), dar nu si din mapa de subscriptii
    pentru a pastra vechile abonamente la o eventuala revenire a clientului

    4. Primirea mesajelor UDP
    - Se apeleaza o functie de inserare a mesajuli UDP intr-o structura pentru a facilita comunicare server - subscriber. In functie se 
    pasrseaza bufferul cu informatii in functie de ce se cere in cerinta. Afisarea pentru fiecare tip de date este handle-uita in codul
    subscriber-ului, in functia "print_message".
    - Apoi se apeleaza functii de pattern matching in functie de natura topicului. Daca este un topic simplu, adica nu o cale (path care
    poate contine wildcards), se cauta efectiv matchul perfect intre topicuri. Pt pathuri se apeleaza o functie de mathcing.
    -> Functia find_match contine comentariile care explica de ce se face fiecare verificare, dar ideea de baza este:
        Se despart atat topicul, cat si pathul in cuvinte folosind strtok cu despartitorul "/"
        Se compara cuvintele intre ele tinand cont de cazurile "+" si "*" 
        Se returneaza true daca pathul este un match pt topic, false altfel
    
    5. Comanda de exit
    - Daca userul introduce la tastatura comanda exit, serverul elibereaza toate socketurile din epoll si da free la maps si se inchide.

    6. Codul subscriber
    - Functioneaza cam la fel ca serverul la nivel de epoll, dar in loc de 3 socketuri de asteptare, sunt doar 2, caci subscriberul 
    comunica doar cu serverul si cu inputul utilizatorului.
    - In cazul inputului de la tastatura se parseaza comanda si apoi se trimite catre server, folosind functia "parse_input".
    - Daca se activeaza socketul de listen, atunci subscriberul primeste un mesaj cu notificare la un topic la care este abonat si se
    apeleaza functia de afisare a mesajului care tine cont de toate tipurile de date posibile.
    
    







