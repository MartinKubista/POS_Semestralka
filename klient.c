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


void* input_thread(void* arg) {
    Game* game = arg;
    char c;

    while (game->running) {
        if (read(STDIN_FILENO, &c, 1) > 0) {
            pthread_mutex_lock(&game->lock);

            if (!game->showMenu && game->gameStat == GAME_STATE_PLAYING) {
                send(game->socket_fd, &c, 1, 0);

            }

            pthread_mutex_unlock(&game->lock);
        }
    }
    return NULL;
}
void* recv_thread(void* arg) {
    Game* game = arg;
    char buf[BUFFER];

    while (game->running) {
        int n = recv(game->socket_fd, buf, BUFFER - 1, 0);
        if (n <= 0) {
            game->running = 0;
            break;
        }

        buf[n] = 0;

        pthread_mutex_lock(&game->lock);
        readData(game, buf);

        if (game->gameStat == GAME_STATE_CANCEL) {
            game->running = 0;
        }
        pthread_mutex_unlock(&game->lock);
    }
    return NULL;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    Game game;
    memset(&game, 0, sizeof(game));
    game.socket_fd = sock;
    game.running = 1;
    game.showMenu = true;

    pthread_mutex_init(&game.lock, NULL);
    initGame(&game);

    pthread_t t_input, t_recv;
    bool isConnected = false;

    char buf[BUFFER];
    while(1){
    while (game.showMenu) {
        system("clear");
        show_menu(&game, buf);

        if(isConnected){
            send(sock, &game.menuSelect, 1, 0);
        }

        if (game.menuSelect == '1') {
            if(isConnected == false){

                pid_t pid = fork();

                if (pid < 0) {
                    perror("fork");
                    exit(1);
                }

                if (pid == 0) {
                    execl("./server", "server", NULL);
                    perror("exec failed");
                    exit(1);
                }

                sleep(1); 

                struct sockaddr_in server = {0};
                server.sin_family = AF_INET;
                server.sin_port = htons(PORT);
                inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

                if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
                    perror("connect");
                    return 1;
                }
                isConnected = true;
                
                send(sock, &game.menuSelect, 1, 0);
            }
            snprintf(buf, BUFFER,
                "MODE=%c\nWORLD=%c\nWIDTH=%d\nHEIGHT=%d\nTIME=%d\n",
                game.mode, game.world,
                game.width, game.height, game.time);
            send(sock, buf, strlen(buf), 0);
            game.gameStat = GAME_STATE_GAMECREATE;
        }
        else if (game.menuSelect == '2' &&
                 game.gameStat == GAME_STATE_GAMECREATE) {

            game.gameStat = GAME_STATE_PLAYING;
            game.showMenu = false;
        }
        else if (game.menuSelect == '3') {
            snprintf(buf, BUFFER,
                "QUIT=%c\n",
                1);
            send(sock, buf, strlen(buf), 0);
            close(sock);
            return 0;
        }
    }
    raw_on(&game);

    game.running = 1;
    pthread_create(&t_input, NULL, input_thread, &game);
    pthread_create(&t_recv, NULL, recv_thread, &game);

    while (game.running) {
        pthread_mutex_lock(&game.lock);

        system("clear");

        if(game.gameStat == GAME_STATE_PLAYING){
            printGame(&game);
        }

        pthread_mutex_unlock(&game.lock);
        usleep(16000); 
    }

    pthread_join(t_input, NULL);
    pthread_join(t_recv, NULL);


  }
    raw_off(&game);

    if (game.objects) {
        free(game.objects);
        game.objects = NULL;
    }

    pthread_mutex_destroy(&game.lock);
    close(sock);

    return 0;
}
