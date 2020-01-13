#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MESSAGE_SIZE 1024
#define PORT 8081

char msg[MESSAGE_SIZE];

char * message_JOIN_GAME(char *buf) {
  strcpy(buf, "0");
};

char * message_MOVE(char *buf, char *direction) {
  strcpy(buf, "1");
  strcat(buf, direction);
};


void *recvmg(void *my_sock){
  int sock = *((int *)my_sock);
  int len;

  while((len = recv(sock,msg,MESSAGE_SIZE,0)) > 0) {
    msg[len] = '\0';
    printf("MESSAGE FROM SERVER: %s \n", msg);
  }
}

int main(int argc){
  pthread_t recvt;
  int len;
  int sock;
  char send_msg[MESSAGE_SIZE];
  struct sockaddr_in server_ip;

  sock = socket( AF_INET, SOCK_STREAM,0);
  server_ip.sin_port = htons(PORT);
  server_ip.sin_family= AF_INET;
  server_ip.sin_addr.s_addr = inet_addr("127.0.0.1");
  if( (connect( sock ,(struct sockaddr *)&server_ip,sizeof(server_ip))) == -1 ) {
      printf("CONNECTION FAILED \n");
  }

  pthread_create(&recvt, NULL, (void *)recvmg, &sock);

  while(fgets(msg, MESSAGE_SIZE, stdin) > 0) {
    strcpy(send_msg, msg);
    len = write(sock, send_msg, strlen(send_msg));
    if(len < 0) {
        printf("SENDING FAILURE \n");
    }
  }

  pthread_join(recvt,NULL);
  close(sock);
  return 0;
}