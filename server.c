#include <sys/socket.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define MIN_CLIENT_COUNT 1
#define MAX_CLIENT_COUNT 3
#define MAX_FOOD_COUNT 5
#define MAP_SIZE 30
#define MAX_SCORE 3
#define MAX_USERNAME_LENGTH 16
#define PORT 8081
#define GAME_STATUS_ACTIVE 0
#define GAME_STATUS_INACTIVE 0
#define GAME_STATUS_ACTIVE 1
#define MESSAGE_SIZE 1024

pthread_mutex_t mutex;
int clients[MAX_CLIENT_COUNT];
struct Game game;
int n=0;
char available_symbols[] = {'A', 'B', 'C', 'D'};

struct Game {
  char game_map[MAP_SIZE][MAP_SIZE + 2];
  char usernames[MAX_CLIENT_COUNT][MAX_USERNAME_LENGTH + 1];
  int scores[MAX_CLIENT_COUNT];
  char symbols[MAX_CLIENT_COUNT];
  int player_statuses[MAX_CLIENT_COUNT];
  int status;
  int player_count;
  int food_count;

  pthread_mutex_t mutex;
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

    for (int i = 0; i < game.player_count; i ++) {
      game.scores[i] = 0;
      game.player_statuses[i] = 0;
    }

    game.status = GAME_STATUS_INACTIVE;
    game.player_count = 0;
}

int get_client_id(int sock){
  for(int i = 0; i < MAX_CLIENT_COUNT; i++) {
    if (clients[i] == sock) {
      return i;
    }
  }

  return -1;
}

int get_id_by_syboml(char symbol){
  for(int i = 0; i < MAX_CLIENT_COUNT; i++) {
    if (game.symbols[i] == symbol) {
      return i;
    }
  }

  return -1;
}

void substring(char sub[], char s[], int beginIndex, int endIndex) {
   strncpy(sub, s + beginIndex, endIndex - beginIndex);
   sub[endIndex - beginIndex] = '\0';
}

void get_map_row(char *buf, int row) {
  strcpy(buf, game.game_map[row]);
}

int find_x_location(char symbol) {
  for(int y = 0; y < MAP_SIZE; y++) {
    for(int x = 0; x < MAP_SIZE; x++) {
      // printf("X: %d Y: %d \n", x, y);
      // printf("%c", game.game_map[x][y]);
      if (game.game_map[x][y] == symbol) {
        return y;
      }
    }
  }
}

int find_y_location(char symbol) {
  for(int y = 0; y < MAP_SIZE; y++) {
    for(int x = 0; x < MAP_SIZE; x++) {
      if (game.game_map[x][y] == symbol) {
        return x;
      }
    }
  }

  return -1;
}

void generate_food() {
  for(;;) {
    int rand_x = rand() % MAP_SIZE + 1;
    int rand_y = rand() % MAP_SIZE + 1;

    if (game.game_map[rand_x][rand_y] == ' ') {
      game.game_map[rand_x][rand_y] = '@';
      break;
    }
  }
}

int * get_all_x_locations() {
  int locations[MAX_CLIENT_COUNT];
  printf("SEARCHING ALL X LOCATIONS \n");

  for (int i = 0; i < game.player_count; i++) {
    printf("X location for symbol %c: %d \n", game.symbols[i], find_x_location(game.symbols[i]));

    locations[i] = find_x_location(game.symbols[i]);
  }

  return locations;
}

int * get_all_y_locations() {
  int locations[MAX_CLIENT_COUNT];
  printf("SEARCHING ALL Y LOCATIONS \n");

  for (int i = 0; i < game.player_count; i++) {
    printf("Y location for symbol  %c: %d \n", game.symbols[i], find_y_location(game.symbols[i]));
    locations[i] = find_y_location(game.symbols[i]);
  }

  return locations;
}

void print_map(struct Game game) {
  char buf[100];

  for (int i = 0; i < MAP_SIZE; i++) {
    get_map_row(buf, i);
    printf(buf);
  }

  printf("\n");
}

void print_stats(struct Game game) {
  for (int i = 0; i < game.player_count; i++) {
    printf("ID: %d, USERNAME %s, SYMOL: %c , SCORE: %d \n", i, game.usernames[i], game.symbols[i], game.scores[i]);  
  }
}


int check_username(char *username){
  for (int i = 0; i < game.player_count; i ++) {
    if (strcmp(username, game.usernames[i]) == 0) {
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
                printf("FAIL \n");
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
                printf("FAIL \n");
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
                printf("FAIL \n");
          continue;
      }
    }
    pthread_mutex_unlock(&mutex);
}

void message_LOBBY_INFO(char *buf) {
  strcpy(buf, "2");

  char *count_str[12];
  sprintf(count_str, "%d", game.player_count);
  strcat(buf, count_str);

  for (int i=0; i < game.player_count; i++) {
      strcat(buf, game.usernames[i]);
      int len = strlen(game.usernames[i]);

      if (len < 16) {
        strcat(buf, "|");
      };
  }
};

void message_GAME_IN_PROGRESS(char *buf) {
  strcpy(buf, "3");
};

void message_USERNAME_TAKEN(char *buf) {
  strcpy(buf, "4");
};

void message_GAME_START(char *buf) {
  char *count_str[12];
  sprintf(count_str, "%d", game.player_count);
  char *width_str[12];
  sprintf(width_str, "%d", MAP_SIZE);
  char *height_str[12];
  sprintf(height_str, "%d", MAP_SIZE);
  

  strcpy(buf, "5");
  strcat(buf, count_str);

  for (int i=0; i < game.player_count; i++) {
      strcat(buf, game.usernames[i]);
      int len = strlen(game.usernames[i]);

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

void message_MAP_ROW(char *buf, int row_num, char row[MAP_SIZE + 2]) {
  char row_num_str[12];
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

void message_GAME_UPDATE(char *buf) {
    char player_count_str[12];
    sprintf(player_count_str, "%d", game.player_count);

    strcpy(buf, "7");
    strcat(buf, player_count_str);

    for (int i = 0; i < game.player_count; i++) {   
      char x[12];
      sprintf(x, "%d", find_x_location(game.symbols[i]));

      char y[12];
      sprintf(y, "%d",  find_y_location(game.symbols[i]));

      char points[12];
      sprintf(points, "%d", game.scores[i]);


      if (game.player_statuses[i] == 1) {
        strcat(buf, "000");
      } else {
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
      }
      
      if (game.player_statuses[i] == 1) {
        strcat(buf, "000");
      } else {
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

      switch (strlen(points)) {
        case 1:
          strcat(buf, "00");
          strcat(buf, points);
          break;
        case 2:
          strcat(buf, "0");
          strcat(buf, points);
          break;
        case 3:
          strcat(buf, points);
          break;
      }
    }

    int food_count = 0;
    for(int y = 0; y < MAP_SIZE; y++) {
      for(int x = 0; x < MAP_SIZE; x++) {
        if (game.game_map[x][y] == '@') {
          food_count++;
        }
      }
    }

    char food_count_str[12];
    sprintf(food_count_str, "%d", food_count);
    switch (strlen(food_count_str)) {
      case 1:
        strcat(buf, "00");
        strcat(buf, food_count_str);
        break;
      case 2:
        strcat(buf, "0");
        strcat(buf, food_count_str);
        break;
      case 3:
        strcat(buf, food_count_str);
        break;
    }

    for(int y = 0; y < MAP_SIZE; y++) {
      for(int x = 0; x < MAP_SIZE; x++) {
        if (game.game_map[x][y] == '@') {
          char food_x_coord[12];
          char food_y_coord[12];
          sprintf(food_x_coord, "%d", y);
          sprintf(food_y_coord, "%d", x);

          switch (strlen(food_x_coord)) {
            case 1:
              strcat(buf, "00");
              strcat(buf, food_x_coord);
              break;
            case 2:
              strcat(buf, "0");
              strcat(buf, food_x_coord);
              break;
            case 3:
              strcat(buf, food_x_coord);
              break;
          }

          switch (strlen(food_y_coord)) {
            case 1:
              strcat(buf, "00");
              strcat(buf, food_y_coord);
              break;
            case 2:
              strcat(buf, "0");
              strcat(buf, food_y_coord);
              break;
            case 3:
              strcat(buf, food_y_coord);
              break;
          }
        }
      }
    }
};

void message_PLAYER_DEAD(char *buf) {
  strcpy(buf, "8");
};

void message_GAME_END(char *buf) {
  char *player_count_str[12];
  sprintf(player_count_str, "%d", game.player_count);

  strcpy(buf, "9");

  strcat(buf, player_count_str);

  for (int i=0; i < game.player_count; i++) {
      strcat(buf, game.usernames[i]);
      int len = strlen(game.usernames[i]);

      if (len < 16) {
        strcat(buf, "|");
      };
      

      char *result_str[12];
      sprintf(result_str, "%d", game.scores[i]);
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

    if (check_username(username) == 1) {
        message_USERNAME_TAKEN(resp);
        send_to_one(resp, sock);
        return;
    } else {
      game.player_count += 1;
      game.symbols[id] =  available_symbols[id];
      strcpy(game.usernames[id], username);
      
      message_LOBBY_INFO(resp);
      send_to_all(resp);

      if (game.player_count == MAX_CLIENT_COUNT) {
        game.status = GAME_STATUS_ACTIVE;
        message_GAME_START(resp);
        send_to_all(resp);

        for (int i = 0; i < MAP_SIZE; i++) {
          char row[MAP_SIZE + 2];
          char buf[MESSAGE_SIZE];

          get_map_row(row, i);
          message_MAP_ROW(buf, i, row);
          
          sleep(1);
          send_to_all(buf);
        }
      }
    }
  }  
}

void handle_MOVE(char *msg, int sock) {
  if (game.status != GAME_STATUS_ACTIVE) {
    return;
  }

  char resp[MESSAGE_SIZE];
  int id = get_client_id(sock);
  int other_id;
  char direction = msg[1];
  char symbol = game.symbols[id];
  int current_x_loc = find_x_location(symbol);
  int current_y_loc = find_y_location(symbol);

  printf("DIRECTION: %c \n", direction);
  printf("ID: %d \n", id);
  printf("SYMBOL: %c \n", symbol);
  printf("LOCATIONS: X - %d ; Y - %d \n", current_x_loc, current_y_loc);

  int new_x_loc;
  int new_y_loc;

  switch (direction) {
    case 'L':
      new_x_loc = current_x_loc - 1;
      new_y_loc = current_y_loc;
      break;
    case 'R':
      new_x_loc = current_x_loc + 1;
      new_y_loc = current_y_loc;
      break;
    case 'U':
      new_x_loc = current_x_loc;
      new_y_loc = current_y_loc - 1;
      break;
    case 'D':
      new_x_loc = current_x_loc;
      new_y_loc = current_y_loc + 1;
      break;
    default:
      return;
  }
  printf("NEW LOCATIONS: X - %d ; Y - %d \n", new_x_loc, new_y_loc);

  char symbol_at_new_loc = game.game_map[new_y_loc][new_x_loc];
  switch (symbol_at_new_loc) {
    case '-':
      return;
    case '|':
      return;
    case '@':
      game.game_map[current_y_loc][current_x_loc] = ' ';
      game.game_map[new_y_loc][new_x_loc] = symbol;
      game.scores[id] += 1;
      generate_food();
      
      break;
    case ' ':
      game.game_map[current_y_loc][current_x_loc] = ' ';
      game.game_map[new_y_loc][new_x_loc] = symbol;

      break;
    default:
      if ((symbol_at_new_loc > (game.player_count + '@')) || (symbol_at_new_loc < 'A')) {
        return;
      }

      other_id = get_id_by_syboml(symbol_at_new_loc);
      
      if (game.scores[id] == game.scores[other_id]) {
        return;
      }

      if (game.scores[id] > game.scores[other_id]) {
        // printf("Player with id: %d; sybmol: %c; score: %d ate player with id: %d; sybmol: %c; score: %d", id, symbol, game.scores[id], other_id, symbol_at_new_loc, game.scores[other_id]);
        game.game_map[current_y_loc][current_x_loc] = ' ';
        game.game_map[new_y_loc][new_x_loc] = symbol;
        game.scores[id] += game.scores[other_id];
        game.player_statuses[other_id] = 1;
        
        message_PLAYER_DEAD(resp);
        send_to_one(resp, clients[other_id]);
      } else {
        // printf("Player with id: %d; sybmol: %c; score: %d is eaten by player with id: %d; sybmol: %c; score: %d", id, symbol, game.scores[id], other_id, symbol_at_new_loc, game.scores[other_id]);
        game.game_map[current_y_loc][current_x_loc] = ' ';
        game.scores[other_id] += game.scores[id];
        game.player_statuses[id] = 1;
        message_PLAYER_DEAD(resp);
        send_to_one(resp, clients[id]);
      }

      break;
  }

  message_GAME_UPDATE(resp);
  send_to_all(resp);

  if ( (game.scores[other_id] >= MAX_SCORE) || (game.scores[id] >= MAX_SCORE)) {
    message_GAME_END(resp);
    send_to_all(resp);
  }

  print_map(game);
  print_stats(game);
}

void *recvmg(void *client_sock){
  int sock = *((int *)client_sock);
  char msg[MESSAGE_SIZE];
  int len;
  while((len = recv(sock,msg,MESSAGE_SIZE,0)) > 0) {
    char message_type = msg[0];
    msg[len] = '\0';
    
    printf("MESSAGE FROM CLIENT: %s \n", msg);

    pthread_mutex_lock(&game.mutex);
    
    switch (message_type) {
    case '0':
      handle_JOIN_GAME(msg, sock);
      break;
    case '1':
      handle_MOVE(msg, sock);
      break;
    }

    pthread_mutex_unlock(&game.mutex);
  }
}

int main(){
  struct sockaddr_in server_ip;
  pthread_t recvt;
  int sock=0 , client_sock=0;

  server_ip.sin_family = AF_INET;
  server_ip.sin_port = htons(PORT);
  server_ip.sin_addr.s_addr = inet_addr("127.0.0.1");
  sock = socket( AF_INET , SOCK_STREAM, 0 );
  if( bind( sock, (struct sockaddr *)&server_ip, sizeof(server_ip)) == -1 ) {
    printf("BINDING ERROR \n");
  } else {
     printf("STARTED\n");
  }

  if(listen(sock, 30) == -1) {
      printf("FAIL \n");
  }

  rand(time(NULL));  
  new_game();
  // print_map(game);

  while(1){
    if( (client_sock = accept(sock, (struct sockaddr *)NULL,NULL)) < 0 ) {
      printf("SOCK FAIL \n");
    }

    pthread_mutex_lock(&mutex);
    clients[n]= client_sock;
    n++;

    pthread_create(&recvt,NULL,(void *)recvmg,&client_sock);
    pthread_mutex_unlock(&mutex);
  }
  return 0; 

}
