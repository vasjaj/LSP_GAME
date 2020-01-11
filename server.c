#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MESSAGE_SIZE 1024
#define MIN_CLIENT 2
#define MAX_CLIENT 8
#define MAP_SIZE 10
#define MAX_SCORE 100
#define PORT 8081
#define GAME_STATUS_ACTIVE 0;
#define GAME_STATUS_INACTIVE 1;
#define SECOND_BEFORE_START int;

pthread_mutex_t mutex;
int clients[MAX_CLIENT];
int n=0;

struct Game {
  char game_map[MAP_SIZE][MAP_SIZE];
  int scores[MAX_CLIENT][MAX_SCORE];
  int status;
  int player_count;
};


// char * message_JOIN_GAME() {
//   return 1;
// };

// char * message_LOBBY_INFO(int player_count, char *username) {
//   char b[MESSAGE_SIZE];

//   sprintf(b, "2%d", player_count);
//   sprintf(b + strlen(b))
//   return b;
// };

// char * message_GAME_IN_PROGRESS() {
//   return "3";
// };

// char * message_USERNAME_TAKEN() {
//   return "4";
// };

// char * message_GAME_START() {
//   return "";
// };

// char * message_MAP_ROW() {
//   return 1;
// };

// char * message_GAME_UPDATE() {
//   return 1;
// };

// char * message_PLAYER_DEAD() {
//   return 1;
// };

// char * message_GAME_END() {
//   return 1;
// };

// char * message_() {
//   return 1;
// };



void sendtoall(char *msg,int curr){
    int i;
    pthread_mutex_lock(&mutex);
    for(i = 0; i < n; i++) {
        if(clients[i] != curr) {
            if(send(clients[i],msg,strlen(msg),0) < 0) {
                printf("sending failure \n");
                continue;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

void *recvmg(void *client_sock){
    int sock = *((int *)client_sock);
    char msg[500];
    int len;
    while((len = recv(sock,msg,500,0)) > 0) {
        msg[len] = '\0';
        sendtoall(msg,sock);
    }

}

int main(){
    struct sockaddr_in ServerIp;
    pthread_t recvt;
    int sock=0 , Client_sock=0;

    ServerIp.sin_family = AF_INET;
    ServerIp.sin_port = htons(PORT);
    ServerIp.sin_addr.s_addr = inet_addr("127.0.0.1");
    sock = socket( AF_INET , SOCK_STREAM, 0 );
    if( bind( sock, (struct sockaddr *)&ServerIp, sizeof(ServerIp)) == -1 )
        printf("cannot bind, error!! \n");
    else
        printf("Server Started\n");

    if( listen( sock ,20 ) == -1 )
        printf("listening failed \n");

    while(1){
        if( (Client_sock = accept(sock, (struct sockaddr *)NULL,NULL)) < 0 )
            printf("accept failed  \n");
        pthread_mutex_lock(&mutex);
        clients[n]= Client_sock;
        n++;
        // creating a thread for each client 
        pthread_create(&recvt,NULL,(void *)recvmg,&Client_sock);
        pthread_mutex_unlock(&mutex);
    }
    return 0; 

}