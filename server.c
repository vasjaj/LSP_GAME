#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define MESSAGE_SIZE 1024
#define MIN_CLIENT 1
#define MAX_CLIENT 3
#define MAX_FOOD_COUNT 5
#define MAP_SIZE 10
#define MAX_SCORE 100
#define MAX_USERNAME_LENGTH 16
#define PORT 8080
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
char available_symbols[] = {'A', 'B', 'C', 'D'};

struct Game {
  char game_map[MAP_SIZE][MAP_SIZE + 2];
  char usernames[MAX_CLIENT][MAX_USERNAME_LENGTH + 1];
  int scores[MAX_CLIENT];
  char symbols[MAX_CLIENT];
  int player_statuses[MAX_CLIENT];
  int status;
  int player_count;
  int food_count;
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
        // printf("LINE: %s \n", line);
        // printf("LENGTH: %d \n", strlen(line));
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

int get_id_by_syboml(char symbol){
  for(int i = 0; i < MAX_CLIENT; i++) {
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
      // printf("X: %d Y: %d \n", x, y);
      // printf("%c", game.game_map[x][y]);
      if (game.game_map[x][y] == symbol) {
        return x;
      }
    }
  }

  return -1;
}

int * get_all_x_locations() {
  int locations[MAX_CLIENT];
  printf("SEARCHING ALL X LOCATIONS \n");

  for (int i = 0; i < game.player_count; i++) {
    printf("X location for symbol %c: %d \n", game.symbols[i], find_x_location(game.symbols[i]));

    locations[i] = find_x_location(game.symbols[i]);
  }

  return locations;
}

int * get_all_y_locations() {
  int locations[MAX_CLIENT];
  printf("SEARCHING ALL Y LOCATIONS \n");

  for (int i = 0; i < game.player_count; i++) {
    printf("Y location for symbol  %c: %d \n", game.symbols[i], find_y_location(game.symbols[i]));
    locations[i] = find_y_location(game.symbols[i]);
  }

  return locations;
}

// int * get_all_food_locations() {
//   int locations[MAX_CLIENT];

//   for (int i = 0; i < game.player_count; i++) {
//     locations[i] = find_y_location(game.symbols[i]);
//   }
// }


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

void print_clients(){
  printf("COUNT: %d \n", game.player_count);
  for (int i = 0; i < game.player_count; i ++) {
    printf("USERNAME %s \n", game.usernames[i]);
  }
  printf("\n");
}

int check_username(char *username){
  for (int i = 0; i < game.player_count; i ++) {
    // printf("COMPARING USERNAME: %s and %s \n", username, game.usernames[i]);
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

void  message_GAME_IN_PROGRESS(char *buf) {
  strcpy(buf, "3");
};

void message_USERNAME_TAKEN(char *buf) {
  strcpy(buf, "4");
};

void message_GAME_START(char *buf, int player_count, char usernames[MAX_CLIENT][MAX_USERNAME_LENGTH + 1], int map_height, int map_width) {
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

void message_MAP_ROW(char *buf, int row_num, char row[MAP_SIZE + 2]) {
  char row_num_str[12];
  sprintf(row_num_str, "%d", row_num);
  // printf("NUM: %s \n", row_num_str);
  strcpy(buf, "6");
  // printf("BUF: %s \n", buf);
  
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
  // printf("adding row \n");
  // printf("ROW: %s \n", row);
  strcat(buf, row);
  // printf("added row \n");
};

void message_GAME_UPDATE(char *buf) {
    // printf("IN GAME UPDATE, PLAYER COUNT: %d; FOOD COUNT: %d \n",  game.player_count, game.food_count);
    char player_count_str[12];
    sprintf(player_count_str, "%d", game.player_count);
    // char food_count_str[12];
    // sprintf(food_count_str, "%d", game.food_count);

    strcpy(buf, "7");
    
    strcat(buf, player_count_str);

    for (int i = 0; i < game.player_count; i++) {   
      char x[12];
      sprintf(x, "%d", find_x_location(game.symbols[i]));

      char y[12];
      sprintf(y, "%d",  find_y_location(game.symbols[i]));

      char points[12];
      sprintf(points, "%d", game.scores[i]);

      // printf("x location: %c, y location: %c, score: %c \n", x, y, points);

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
        // printf("X: %d Y: %d \n", x, y);
        // printf("%c", game.game_map[x][y]);
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
        // printf("X: %d Y: %d \n", x, y);
        // printf("%c", game.game_map[x][y]);
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

void message_GAME_END(char *buf, int player_count, char usernames[MAX_CLIENT][MAX_USERNAME_LENGTH + 1], int results[]) {
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

    // printf("CLIENT USERNAME: %s \n",  username);

    if (check_username(username) == 1) {
        // printf("USERNAME TAKEN");
        message_USERNAME_TAKEN(resp);
        send_to_one(resp, sock);
        return;
    } else {
      game.player_count += 1;
      game.symbols[id] =  available_symbols[id];
      strcpy(game.usernames[id], username);
      

      // printf("PLAYER COUNT: %d \n",  game.player_count);
      // printf("PLAYER USERNAME: %s \n",  game.usernames[id]);
      // printf("PLAYER SYMBOL: %c \n", game.symbols[id]);

      if (game.player_count == 2) {
        // print_clients();
        game.status = GAME_STATUS_ACTIVE;
        message_GAME_START(resp, game.player_count, game.usernames, MAP_SIZE, MAP_SIZE);
        send_to_all(resp);

        for (int i = 0; i < MAP_SIZE; i++) {
          char row[MAP_SIZE + 2];
          char buf[MESSAGE_SIZE];

          get_map_row(row, i);
          message_MAP_ROW(buf, i, row);
          
          // sleep(1);
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
      
      message_GAME_UPDATE(resp);
      send_to_all(resp);
      
      break;
    case ' ':
      game.game_map[current_y_loc][current_x_loc] = ' ';
      game.game_map[new_y_loc][new_x_loc] = symbol;
      // printf("TEST: x %d, y %d \n", x_locations[0], y_locations[0]);
      int *food_x_coords;
      int *foo_y_coords;
      
      break;
    default:
      if ((symbol_at_new_loc > (game.player_count + '@')) || (symbol_at_new_loc < 'A')) {
        return;
      }

      int other_id = get_id_by_syboml(symbol_at_new_loc);
      
      if (game.scores[id] == game.scores[other_id]) {
        return;
      }

      if (game.scores[id] > game.scores[other_id]) {
        // printf("Player with id: %d; sybmol: %c; score: %d ate player with id: %d; sybmol: %c; score: %d", id, symbol, game.scores[id], other_id, symbol_at_new_loc, game.scores[other_id]);
        game.game_map[current_y_loc][current_x_loc] = ' ';
        game.game_map[new_y_loc][new_x_loc] = symbol;
        game.scores[id] += game.scores[other_id];
        game.player_statuses[other_id] = 1;
      } else {
        // printf("Player with id: %d; sybmol: %c; score: %d is eaten by player with id: %d; sybmol: %c; score: %d", id, symbol, game.scores[id], other_id, symbol_at_new_loc, game.scores[other_id]);
        game.game_map[current_y_loc][current_x_loc] = ' ';
        game.scores[other_id] += game.scores[id];
        game.player_statuses[id] = 1;
        message_PLAYER_DEAD(resp);
        send_to_one(resp, sock);
      }
      
      break;
  }
  
  message_GAME_UPDATE(resp);
  send_to_all(resp);
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
    // printf("%d %d \n",find_x_location('C'), find_y_location('C'));

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
    print_map(game);

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
