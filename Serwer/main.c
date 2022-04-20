#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>


#define QUEUE_SIZE 10
#define USER_LIMIT 100
#define ROOM_LIMIT 15

void remove_endl(char *s)
{
    for(int i=0;s[i];i++){
        if(s[i]=='\r'){
            s[i]='\0';
        }
        else if(s[i]=='\n'){
            s[i]='\0';
            break;
        }
	}
}

//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
    pthread_mutex_t *mutex_msg; //tablica mutexow do wysylania dla kazdego usera
    pthread_mutex_t *mutex_names; //mutex nazw uzytkownikow
    pthread_mutex_t *mutex_rname; //mutex nazw pokojow
    pthread_mutex_t *mutex_assign; //mutex przydzialu
    int *con; //tablica deskryptorow
    char (*username)[31]; //tablica nazw uzytkownikow
    int *rooms; //tablica przydzialu uzytkownikow do pokojow
    char (*room_name)[31]; //tablica nazw pokojow
    int id; //id uzytkownika
};

//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ThreadBehavior(void *t_data)
{
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data; //wskazniki do wszystkich danych
    char msg[500];
    char msgcat[600];
	int check;
    int id = th_data->id;
    char login[30];
    int currentRoom;
    bool roomScanner[ROOM_LIMIT];
    int conExit = 0;
    bool isOwner = false;
    bool quiet = false;

    memset(login,0, sizeof(login));
    check = read(th_data->con[id], login, 30); //odebranie loginu
    remove_endl(login);
    if(strcmp("$DISCONNECT",login)==0 || check<1){
        close(th_data->con[id]);
        pthread_mutex_lock(th_data->mutex_assign);
        th_data->con[id] = -2;
        pthread_mutex_unlock(th_data->mutex_assign);
        printf("User %d has been disconnected\n",id);
        //czyszczenie pamieci
        free(t_data);
        pthread_exit(NULL);
    }
    printf("User %d has entered username: %s\n",id, login);
    strncpy(th_data->username[id], login, 31);
    while(1){
        quiet = false;
        currentRoom=-1;
        isOwner = false;
        for(int i=0;i<ROOM_LIMIT;i++){ //oczyszczanie listy pokojow
            roomScanner[i]=false;
        }
        pthread_mutex_lock(th_data->mutex_assign);
        for(int i=0;i<USER_LIMIT;i++){
            if(th_data->rooms[i] < 0) continue;
            roomScanner[th_data->rooms[i]] = true;
        }
        pthread_mutex_lock(th_data->mutex_rname);
        for(int i=0;i<ROOM_LIMIT;i++){ //room garbage collecotor
            if(roomScanner[i]==false && th_data->room_name[i][0]!='\0'){
                memset(th_data->room_name[i],0, sizeof(th_data->room_name[i]));
                th_data->room_name[i][0] = '\0';
            }
        }
        pthread_mutex_unlock(th_data->mutex_rname);
        pthread_mutex_unlock(th_data->mutex_assign);

		memset(msg,0, sizeof(msg)); //wypisywanie listy otwartych pokojow
		msg[0] = '\0';
		pthread_mutex_lock(th_data->mutex_rname);
		for(int i=0;i<ROOM_LIMIT;i++){
            if(th_data->room_name[i][0]==0) continue;
            strcat(msg,th_data->room_name[i]);
            strcat(msg,";");
		}
		strcat(msg,"\n");
		pthread_mutex_unlock(th_data->mutex_rname);
        check = write(th_data->con[id], msg, strlen(msg));
        if(check<0){
			printf("E: Blad wysylania listy serwerow\n");
			break;
        }
        memset(msg,0, sizeof(msg));
        msg[0] = '\0';
        check = read(th_data->con[id], msg, 30); //przydzielanie do wybranego pokoju
        remove_endl(msg);
        if(strcmp("$DISCONNECT",msg)==0 || check<1){
            break;
        }
        if(strcmp("$REFRESH",msg)==0){
            continue;
        }
        if(msg[0]=='$'){ //tworzenie nowego pokoju
            if(msg[1]=='\0' || msg[1]==' '){
                printf("User %d has failed to create a room: Invalid name\n",id);
                check = write(th_data->con[id], "$FAIL_CREATE_NAME\n", 18);
                if(check<0){
                    printf("E: Blad wyslania komunikatu\n");
                    break;
                }
                continue;
            }
            pthread_mutex_lock(th_data->mutex_rname);
            for(int i=0;i<ROOM_LIMIT;i++){
                if(th_data->room_name[i][0]=='\0'){
                    pthread_mutex_lock(th_data->mutex_assign);
                    char* substr =msg + 1;
                    strncpy(th_data->room_name[i], substr, 31);
                    th_data->rooms[id] = i;
                    pthread_mutex_unlock(th_data->mutex_assign);
                    currentRoom=i;
                    isOwner = true;
                    break;
                }
            }
            pthread_mutex_unlock(th_data->mutex_rname);
            if(currentRoom==-1) {
                printf("User %d has failed to create a room: Limit exceeded\n",id);
                check = write(th_data->con[id], "$FAIL_CREATE_LIMIT\n", 19);
                if(check<0){
                    printf("E: Blad wyslania komunikatu\n");
                    break;
                }
                continue;
            }
        }
        else{ //dolaczanie do istniejacego
            if(msg[0]!='\0' || msg[0]!=' '){
                for(int i=0;i<ROOM_LIMIT;i++){
                    if(strcmp(th_data->room_name[i],msg)==0){
                        pthread_mutex_lock(th_data->mutex_assign);
                        th_data->rooms[id] = i;
                        currentRoom=i;
                        pthread_mutex_unlock(th_data->mutex_assign);
                    }
                }
            }
            if(currentRoom==-1) {
                printf("User %d has failed to choose a valid room\n",id);
                check = write(th_data->con[id], "$FAIL_JOIN_INVALID\n", 19);
                if(check<0){
                    printf("E: Blad wyslania komunikatu\n");
                    break;
                }
                continue;
            }
        }
        printf("User %d has joined room %s\n",id, th_data->room_name[currentRoom]);
        check = write(th_data->con[id], "$OK_JOIN\n", 9);
        if(check<0){
            printf("E: Blad wyslania komunikatu\n");
            break;
        }
        while(th_data->rooms[id]==currentRoom){ //wysylanie i odbieranie wiadomosci
            if(currentRoom<0) break;
            if(th_data->con[id]<0){
                conExit = 1;
                break;
            }
            memset(msg,0, sizeof(msg));
            msg[0] = '\0';
            memset(msgcat,0, sizeof(msgcat));
            msgcat[0] = '\0';
            check = read(th_data->con[id], msg, 300);
            if(check<1){
                conExit = 1;
                break;
            }
            remove_endl(msg);
            if(th_data->rooms[id]!=currentRoom){
                quiet = true;
                break;
            }
            if(strcmp("$LEAVE",msg)==0){
                pthread_mutex_lock(th_data->mutex_assign);
                th_data->rooms[id] = -1;
                pthread_mutex_unlock(th_data->mutex_assign);
                break;
            }
            else if(strcmp("$DISCONNECT",msg)==0){
                conExit = 1;
                break;
            }
            else if(strncmp(msg, "$REM ", 5) == 0){ //banowanie z pokoju
                if(isOwner==false){
                    pthread_mutex_lock(&th_data->mutex_msg[id]);
                    check = write(th_data->con[id], "$FAIL_REM_NOAUTH\n", 17);
                    pthread_mutex_unlock(&th_data->mutex_msg[id]);
                    if(check<0){
                        printf("E: Blad wyslania komunikatu\n");
                        conExit = 1;
                        break;
                    }
                    continue;
                }
                char* substr =msg + 5;
                bool flag = false;
                for(int i = 0; i<USER_LIMIT;i++){
                    if(strncmp(th_data->username[i],substr,40)==0){
                        flag = true;
                        pthread_mutex_lock(th_data->mutex_assign);
                        th_data->rooms[i] = -1;
                        pthread_mutex_unlock(th_data->mutex_assign);
                        pthread_mutex_lock(&th_data->mutex_msg[i]);
                        check = write(th_data->con[i], "$ROOM_REMOVED\n", 14);
                        pthread_mutex_unlock(&th_data->mutex_msg[i]);
                        if(check<0){
                            printf("E: Blad wyslania komunikatu\n");
                            break;
                        }
                        printf("User %d in %s has removed user %d\n",id,th_data->room_name[currentRoom],i);
                        break;
                    }
                }
                if(flag==false){
                    pthread_mutex_lock(&th_data->mutex_msg[id]);
                    check = write(th_data->con[id], "$FAIL_REM_UNKNOWN\n", 18);
                    pthread_mutex_unlock(&th_data->mutex_msg[id]);
                    if(check<0){
                        printf("E: Blad wyslania komunikatu\n");
                        conExit = 1;
                        break;
                    }
                    continue;
                }
                pthread_mutex_lock(&th_data->mutex_msg[id]);
                check = write(th_data->con[id], "$OK_REM\n", 8);
                pthread_mutex_unlock(&th_data->mutex_msg[id]);
                if(check<0){
                    printf("E: Blad wyslania komunikatu\n");
                    conExit = 1;
                    break;
                }
                continue;
            }
            strcat(msgcat,th_data->username[id]);
            strcat(msgcat,": ");
            strcat(msgcat,msg);
            strcat(msgcat,"\n");
            for(int i=0; i<USER_LIMIT;i++){
                if(i==id || th_data->rooms[i]!=currentRoom ) continue;
                pthread_mutex_lock(&th_data->mutex_msg[i]);
                check = write(th_data->con[i], msgcat, strlen(msgcat));
                pthread_mutex_unlock(&th_data->mutex_msg[i]);
                if(check<0){
                    printf("User %d in %s has failed to send a message to user %d\n",id,th_data->room_name[currentRoom],i);
                    //connection drop
                }
            }
        }
        if(conExit==1){
            break;
        }
        printf("User %d has been removed from room\n",id);
        if(quiet==true) continue;
        pthread_mutex_lock(&th_data->mutex_msg[id]);
        check = write(th_data->con[id], "$ROOM_LEFT\n", 11);
        pthread_mutex_unlock(&th_data->mutex_msg[id]);
        if(check<0){
            printf("E: Blad wyslania komunikatu\n");
            break;
        }
        continue;
	}
    //sprzatanie
    check = write(th_data->con[id], "$ACK_DISCONNECT\n", 16);
    close(th_data->con[id]);
    pthread_mutex_lock(th_data->mutex_assign);
    th_data->rooms[id] = -1;
    pthread_mutex_unlock(th_data->mutex_assign);
    memset(th_data->username[id],0, sizeof(th_data->username[id]));
    th_data->username[id][0] = '\0';
    pthread_mutex_lock(&th_data->mutex_msg[id]);
    th_data->con[id] = -2;
    pthread_mutex_unlock(&th_data->mutex_msg[id]);
    printf("User %d has been disconnected\n",id);
	//czyszczenie pamieci
    free(t_data);
    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor,pthread_mutex_t *m1,pthread_mutex_t* m2,pthread_mutex_t* m3,pthread_mutex_t* m4,
 int *con, char username[][31], int *rooms, char room_name[][31])  {
    //wynik funkcji tworzącej wątek
    int create_result = 0;

    int id=-1; //przydzial user_id
    for(int i=0;i<USER_LIMIT;i++){
        if(con[i]==-2){
            id = i;
            break;
        }
    }
    if(id>-1){
        con[id] = connection_socket_descriptor;
    }
    else{
        printf("E: Osiagnieto limit uzytkownikow\n");
        return;
    }
    printf("New user detected with id = %d\n",id);

    //uchwyt na wątek
    pthread_t newThread;
    //dane, które zostaną przekazane do wątku

    //dynamiczne utworzenie instancji struktury thread_data_t o nazwie t_data (+ w odpowiednim miejscu zwolnienie pamięci)
    struct thread_data_t *t_data = malloc(sizeof(struct thread_data_t) );
    //wypełnienie pól struktury
    t_data->mutex_msg = m1;
    t_data->mutex_names = m2;
    t_data->mutex_rname = m3;
    t_data->mutex_assign = m4;
    t_data->con = con;
    t_data->username = username;
    t_data->rooms = rooms;
    t_data->room_name = room_name;
    t_data->id = id;
    create_result = pthread_create(&newThread, NULL, ThreadBehavior, (void *)t_data);
    if (create_result){
       printf("E: Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       exit(-1);
    }
}

int main(int argc, char* argv[])
{
   int port;
   if(argc!=2){
        printf("Syntax: irc-server <port>\n");
        return 1;
   }
   else{
        port = atoi(argv[1]);
        printf("irc-server\nRunning on port %d\n",port);
   }
   int server_socket_descriptor;
   int connection_socket_descriptor;
   int bind_result;
   int listen_result;
   char reuse_addr_val = 1;
   struct sockaddr_in server_address;
   //mutexy
   pthread_mutex_t m1[USER_LIMIT];
   for (int i = 0; i < USER_LIMIT; i++) pthread_mutex_init(&m1[i], NULL);
   pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
   pthread_mutex_t m3 = PTHREAD_MUTEX_INITIALIZER;
   pthread_mutex_t m4 = PTHREAD_MUTEX_INITIALIZER;
   //tablice dynamiczne
    int con[USER_LIMIT]; //tablica deskryptorow
    char username[USER_LIMIT][31]; //tablica nazw uzytkownikow
    int rooms[USER_LIMIT]; //tablica przydzialu uzytkownikow do pokojow
    char room_name[ROOM_LIMIT][31]; //tablica nazw pokojow
    for(int i=0;i<USER_LIMIT;i++){
        con[i] = -2;
        memset(username[i],0, sizeof(username[i]));
        username[i][0] = '\0';
        rooms[i] = -1;
    }
    for(int i=0;i<ROOM_LIMIT;i++){
        memset(room_name[i],0, sizeof(room_name[i]));
        username[i][0] = '\0';
    }
   //inicjalizacja gniazda serwera
   memset(&server_address, 0, sizeof(struct sockaddr));
   server_address.sin_family = AF_INET;
   server_address.sin_addr.s_addr = htonl(INADDR_ANY);
   server_address.sin_port = htons(port);

   server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
   if (server_socket_descriptor < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
       exit(1);
   }
   setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

   bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
   if (bind_result < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
       exit(1);
   }

   listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
   if (listen_result < 0) {
       fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
       exit(1);
   }

   while(1)
   {
       connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
       if (connection_socket_descriptor < 0)
       {
           fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
           exit(1);
       }

       handleConnection(connection_socket_descriptor,m1, &m2, &m3, &m4, con, username, rooms, room_name);
   }

   close(server_socket_descriptor);
   return(0);
}
