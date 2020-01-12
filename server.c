#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MESSAGE_SIZE 1024
#define MIN_CLIENT 1
#define MAX_CLIENT 3
#define MAP_SIZE 10
#define MAX_SCORE 100
#define MAX_USERNAME_LENGTH 16
#define PORT 8081
#define GAME_STATUS_ACTIVE 0
#define GAME_STATUS_INACTIVE 1
#define SECOND_BEFORE_START int
#define GAME_STATUS_INACTIVE 0
#define GAME_STATUS_ACTIVE 1
#define MESSAGE_SIZE 124

pthread_mutex_t mutex;
int clients[MAX_CLIENT];
struct Game game;
int n=0;

struct Game {
  char game_map[MAP_SIZE][MAP_SIZE];
  char usernames[MAX_CLIENT][MAX_USERNAME_LENGTH + 1];
  int scores[MAX_CLIENT];
  int status;
  int player_count;
};

void new_game() {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("./map.txt", "r");
    if (fp == NULL) {
      printf("Error \n");
    }

    int n = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
        strcpy(game.game_map[n], line);
        n++;
    }
    fclose(fp);

    game.status = GAME_STATUS_INACTIVE;
    game.player_count = 0;
}

int get_client_id(int sock){
  for(int i = 0; i < MAX_CLIENT; i++) {
    if (clients[i] == sock) {
      return i;
    }
  }

  return -1;
}

void substring(char sub[], char s[], int beginIndex, int endIndex) {
   strncpy(sub, s + beginIndex, endIndex - beginIndex);
   sub[endIndex - beginIndex] = '\0';
}


void print_map(struct Game game) {
    printf("PRINTING MAP");
    for (int i  = 0; i < MAP_SIZE; i++) {
      printf("%s", game.game_map[i]);
    }
}

void print_clients(){
  printf("COUNT: %d \n", game.player_count);
  for (int i = 0; i < game.player_count; i ++) {
    printf("USERNAME %s \n", game.usernames[i]);
  }
  printf("================= \n");
}

int check_username(char *username){
  // printf("COMPARING USERNAME: %s \n", username);
  for (int i = 0; i < game.player_count; i ++) {
    if (strcmp(username, game.usernames[i]) == 0) {
      // printf("DUPLICATE: %s and %s", game.usernames[i], username);
      return 1;
    }
  }

  return 0;
}

void send_to_others(char *msg,int curr){
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

void send_to_one(char *msg,int curr){
    int i;
    pthread_mutex_lock(&mutex);
    for(i = 0; i < n; i++) {
        if(clients[i] == curr) {
            if(send(clients[i],msg,strlen(msg),0) < 0) {
                printf("sending failure \n");
                continue;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

void send_to_all(char *msg){
    int i;
    pthread_mutex_lock(&mutex);
    for(i = 0; i < n; i++) {
      if(send(clients[i],msg,strlen(msg),0) < 0) {
          printf("sending failure \n");
          continue;
      }
    }
    pthread_mutex_unlock(&mutex);
}

void message_LOBBY_INFO(char *buf, int player_count, char usernames[MAX_CLIENT][MAX_USERNAME_LENGTH + 1]) {
  strcpy(buf, "2");

  char *count_str[12];
  sprintf(count_str, "%d", player_count);
  strcat(buf, count_str);

  for (int i=0; i < player_count; i++) {
      strcat(buf, usernames[i]);
      int len = strlen(usernames[i]);

      if (len < 16) {
        strcat(buf, "|");
      };
  }
};

char * message_GAME_IN_PROGRESS(char *buf) {
  strcpy(buf, "3");
};

char * message_USERNAME_TAKEN(char *buf) {
  strcpy(buf, "4");
};

char * message_GAME_START(char *buf, int player_count, char usernames[MAX_CLIENT][MAX_USERNAME_LENGTH + 1], int map_height, int map_width) {
  char *count_str[12];
  sprintf(count_str, "%d", player_count);
  char *width_str[12];
  sprintf(width_str, "%d", map_width);
  char *height_str[12];
  sprintf(height_str, "%d", map_height);
  

  strcpy(buf, "5");
  strcat(buf, count_str);

  for (int i=0; i < player_count; i++) {
      strcat(buf, usernames[i]);
      int len = strlen(usernames[i]);

      if (len < 16) {
        strcat(buf, "|");
      };
  }
  
  switch (strlen(height_str)) {
    case 1:
      strcat(buf, "00");
      strcat(buf, height_str);
      break;
    case 2:
      strcat(buf, "0");
      strcat(buf, height_str);
      break;
    case 3:
      strcat(buf, height_str);
      break;
  }


  switch (strlen(width_str)) {
    case 1:
      strcat(buf, "00");
      strcat(buf, width_str);
      break;
    case 2:
      strcat(buf, "0");
      strcat(buf, width_str);
      break;
    case 3:
      strcat(buf, width_str);
      break;
  }
};

char * message_MAP_ROW(char *buf, int *row_num, char *row) {
  char *row_num_str[12];
  sprintf(row_num_str, "%d", row_num);

  strcpy(buf, "6");

  switch (strlen(row_num_str)) {
    case 1:
      strcat(buf, "00");
      strcat(buf, row_num_str);
      break;
    case 2:
      strcat(buf, "0");
      strcat(buf, row_num_str);
      break;
    case 3:
      strcat(buf, row_num_str);
      break;
  }

  strcat(buf, row);
};

char * message_GAME_UPDATE(char *buf, int player_count, int x_player_coords[], int y_player_coords[], int food_count, int x_food_coords[], int y_food_coords[]) {
    char *player_count_str[12];
    sprintf(player_count_str, "%d", player_count);
    char *food_count_str[12];
    sprintf(food_count_str, "%d", food_count);

    strcpy(buf, "7");
    
    strcat(buf, player_count_str);

    for (int i; i < player_count; i++) {
      char *x[12];
      sprintf(x, "%d", x_player_coords[i]);

      char *y[12];
      sprintf(y, "%d", y_player_coords[i]);

      switch (strlen(x)) {
        case 1:
          strcat(buf, "00");
          strcat(buf, x);
          break;
        case 2:
          strcat(buf, "0");
          strcat(buf, x);
          break;
        case 3:
          strcat(buf, x);
          break;
      }
      
       switch (strlen(y)) {
        case 1:
          strcat(buf, "00");
          strcat(buf, y);
          break;
        case 2:
          strcat(buf, "0");
          strcat(buf, y);
          break;
        case 3:
          strcat(buf, y);
          break;
      }
    }

    strcat(buf, food_count_str);
    for (int i; i < food_count; i++) {
      char *x[12];
      sprintf(x, "%d", x_food_coords[i]);

      char *y[12];
      sprintf(y, "%d", y_food_coords[i]);

      switch (strlen(x)) {
        case 1:
          strcat(buf, "00");
          strcat(buf, x);
          break;
        case 2:
          strcat(buf, "0");
          strcat(buf, x);
          break;
        case 3:
          strcat(buf, x);
          break;
      }
      
       switch (strlen(y)) {
        case 1:
          strcat(buf, "00");
          strcat(buf, y);
          break;
        case 2:
          strcat(buf, "0");
          strcat(buf, y);
          break;
        case 3:
          strcat(buf, y);
          break;
      }
    }
};

char * message_PLAYER_DEAD(char *buf) {
  strcpy(buf, "8");
};

char * message_GAME_END(char *buf, int player_count, char usernames[MAX_CLIENT][MAX_USERNAME_LENGTH + 1], int results[]) {
  char *player_count_str[12];
  sprintf(player_count_str, "%d", player_count);

  strcpy(buf, "9");

  strcat(buf, player_count_str);

  for (int i=0; i < player_count; i++) {
      strcat(buf, usernames[i]);
      int len = strlen(usernames[i]);

      if (len < 16) {
        strcat(buf, "|");
      };
      

      char *result_str[12];

      int result = results[i];
      sprintf(result_str, "%d", result);
      printf(result_str);

      switch (strlen(result_str)) {
        case 1:
          strcat(buf, "00");
          strcat(buf, result_str);
          break;
        case 2:
          strcat(buf, "0");
          strcat(buf, result_str);
          break;
        case 3:
          strcat(buf, result_str);
          break;
      }
  }
};

void handle_JOIN_GAME(char *msg, int sock) {
  char resp[MESSAGE_SIZE];
  int id = get_client_id(sock);
  // printf("CLIENT ID: %d \n", id);

  char input[17];
  substring(input, msg, 1, strlen(msg) - 1);

  if (game.status == GAME_STATUS_ACTIVE) {
    message_GAME_IN_PROGRESS(resp);
    send_to_one(resp, sock);
    return;
  }

  if (game.status == GAME_STATUS_INACTIVE) {
    char username[17];
    strcpy(username, input);

    printf("CLIENT USERNAME: %s \n",  username);

    if (check_username(username) == 1) {
        // printf("USERNAME TAKEN");
        message_USERNAME_TAKEN(resp);
        send_to_one(resp, sock);
        return;
    } else {
      game.player_count += 1;
      strcpy(game.usernames[id], username);

      // printf("PLAYER COUNT: %d \n",  game.player_count);
      // printf("CLIENT USERNAME: %s \n",  game.usernames[id]);

      if (game.player_count == 3) {
        // print_clients();
        message_GAME_START(resp, game.player_count, game.usernames, MAP_SIZE, MAP_SIZE);
        send_to_all(resp);
      }
    }
  }  
}

void handle_MOVE(char *msg, int sock) {
  printf("IN MOVE \n");
  send_to_others(msg,sock);
}

void *recvmg(void *client_sock){
    int sock = *((int *)client_sock);
    char msg[MESSAGE_SIZE];
    int len;
    while((len = recv(sock,msg,MESSAGE_SIZE,0)) > 0) {
        char message_type = msg[0];
        msg[len] = '\0';
        
        printf("%d \n", sock);
        printf("MESSAGE FROM CLIENT: %s \n", msg);

        switch (message_type) {
        case '0':
          handle_JOIN_GAME(msg, sock);
          break;
        case '1':
          handle_MOVE(msg, sock);
          break;
        }
    }
}

int main(){
    // new_game();
    // game.player_count = 2;
    // strcpy(game.usernames[0], "username1");
    // strcpy(game.usernames[1], "username2");
    // char *resp;
    // message_GAME_START(resp, game.player_count, game.usernames, MAP_SIZE, MAP_SIZE);
    // printf(resp);
    // print_map(game);


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


    new_game();
    // print_map(game);
    while(1){
        if( (Client_sock = accept(sock, (struct sockaddr *)NULL,NULL)) < 0 ) {
            printf("accept failed  \n");
        }

        pthread_mutex_lock(&mutex);
        clients[n]= Client_sock;
        n++;
        // creating a thread for each client 
        pthread_create(&recvt,NULL,(void *)recvmg,&Client_sock);
        pthread_mutex_unlock(&mutex);
    }
    return 0; 

}
