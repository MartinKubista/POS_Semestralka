#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "hrac.h"

#define PORT 13777
#define BUFFER_SIZE 1024


void* input_thread(void* arg) {//vstup od pouzi a posielanie na server
    Game* game = (Game*)arg;
    char command;
    char buf[BUFFER];

    while (game->showMenu == false) {
      pthread_mutex_lock(&game->lock);
        char c;
            if (read(STDIN_FILENO, &c, 1) > 0){
                send(game->socket_fd, &c, 1, 0);

            } // w, a, s, d
      pthread_mutex_unlock(&game->lock);

        send(game->socket_fd, &command, sizeof(command), 0);
    }
    return NULL;
}
void* receive_thread(void* arg) {
    Game* game = arg;
    char buf[BUFFER];
    int lastX = -1, lastY = -1;

    while (1) {
        pthread_mutex_lock(&game->lock);
        if (game->showMenu || game->gameStat == GAME_STATE_CANCEL) {
            pthread_mutex_unlock(&game->lock);
            break;
        }
        pthread_mutex_unlock(&game->lock);

        int n = recv(game->socket_fd, buf, BUFFER - 1, MSG_DONTWAIT);
        if (n <= 0) {
            usleep(10000);
            continue;
        }

        buf[n] = 0;

        pthread_mutex_lock(&game->lock);
        readData(game, buf);

        if (game->objects && game->obj > 0) {
            if (game->objects[0].x != lastX ||
                game->objects[0].y != lastY) {

                lastX = game->objects[0].x;
                lastY = game->objects[0].y;
                pthread_mutex_unlock(&game->lock);
                printGame(game);
                continue;
            }
        }
        pthread_mutex_unlock(&game->lock);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
  int client_fd;
  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(client_fd <0){
    perror("Vytvorenie socketu zlyhalo");
    return -1;
  }
  printf("Socet vytvoreny\n");

  //nastavenie ip adresu servera
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
if(inet_pton(AF_INET, "127.0.0.1", (struct sockaddr*)&server_addr.sin_addr) < 0){
  perror("Neplatna adresa");
  close(client_fd);
  return -2;
  }
  

  if(connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
    perror("Pripojenie zlyhalo");
    close(client_fd);
    return -3;
  }

  char buffer[BUFFER_SIZE];

    Game game;
    game.socket_fd = client_fd;
    game.running = 1;
    initGame(&game);
    pthread_mutex_init(&game.lock, NULL);
    char buf[BUFFER];
    while (1) {
        system("clear");

        if(game.showMenu == true ){
            show_menu(&game, buf);
            send(game.socket_fd, &game.menuSelect, sizeof(game.menuSelect), 0);
            printf("[CLIENT DEBUG] sent initial menuSelect='%c'\n", game.menuSelect);

            if (game.menuSelect == '1'){
                memset(buf, 0, sizeof(buf));
                snprintf(buf, BUFFER,
                "MODE=%c\nWORLD=%c\nWIDTH=%d\nHEIGHT=%d\nTIME=%d\n",
                game.mode, game.world, game.width, game.height, game.time);
                send(game.socket_fd, buf, strlen(buf), 0);
                game.gameStat = GAME_STATE_GAMECREATE;
                
            } else if (game.menuSelect == '2' && game.gameStat == GAME_STATE_GAMECREATE){
                game.gameStat = GAME_STATE_PLAYING;
                raw_on();
                printf("[CLIENT DEBUG] joined game (new) inGame=%d\n", game.gameStat);
                game.showMenu = false;

            } else if (game.menuSelect == '3' && game.gameStat == GAME_STATE_PAUSED){
                send(game.socket_fd, &game.menuSelect, 1, 0);
                printf("[CLIENT DEBUG] sent resume request menuSelect='%c'\n", game.menuSelect);
                raw_on();
                game.gameStat = GAME_STATE_DELAY; 
                printf("[CLIENT DEBUG] resume: inGame=%d\n", game.gameStat);
                game.showMenu = false;

            } else if (game.menuSelect == '4'){
                break;
            }
        }
        if(game.showMenu == false ){
            if (game.gameStat == GAME_STATE_DELAY)
            {
                printGame(&game);
                sleep(3);
                game.gameStat = GAME_STATE_PLAYING;
                continue;

            }

            pthread_t t_send, t_recv;

            pthread_create(&t_send, NULL, input_thread, &game);
            pthread_create(&t_recv, NULL, receive_thread, &game);

            pthread_join(t_send, NULL);
            pthread_join(t_recv, NULL);
        }
    }
    if (game.objects) {
        free(game.objects);
        game.objects = NULL;
    }
  raw_off();
  close(client_fd);
  printf("Klient ukonceny\n");
  return 0;
}
