#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <sys/select.h>
#include <errno.h>

#include "hra.h"

int main() {
    srand(time(NULL));

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { 
        perror("socket"); return 1; 
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    printf("Server beží na porte %d\n", PORT);

    int client = accept(server_fd, NULL, NULL);
    printf("Klient pripojený\n");

    Game game;
    memset(&game, 0, sizeof(game));

    char input;
    char buf[BUFFER];
    time_t client_disconnect_time = 0;
    bool client_connected = true;


    while (1) {
        if (!game.inGame) {
            if (!game.inGame && game.gameCreate == true) {
                time_t now = time(NULL);
                if (difftime(now, game.initTIme) >= 10 && game.mode == '1') {
                    printf("[SERVER DEBUG] sending timeout after 10 sek\n");
                    if(game.end == true || !client_connected){
                        break;
                    }
                    send(client, "TIMEOVER\n", 9, 0);
                    reset_game(&game);
                    game.initTIme = time(NULL);
                    continue;
                }
                if (game.mode == '2') {
                time_t now = time(NULL);
                int elapsed = (int)(now - game.initTIme) - game.pausedSeconds;

                printf("[SERVER DEBUG] čas: %d / %d sekúnd\n",
                    elapsed, game.maxTime);

                if (elapsed >= game.maxTime) {
                    if(game.end == true || !client_connected){
                        break;
                    }
                    send(client, "TIMEOVER\n", 9, 0);
                    reset_game(&game);
                    continue;
                }
            }
            }

            fd_set rfds;
            struct timeval tv;

            FD_ZERO(&rfds);
            FD_SET(client, &rfds);

            // čakaj max 100 ms
            tv.tv_sec = 0;
            tv.tv_usec = 100000;

            int ret = select(client + 1, &rfds, NULL, NULL, &tv);
            
            if (ret > 0 && FD_ISSET(client, &rfds)) {
                int recv_ret = recv(client, &game.menuSelect, 1, 0);
                if (recv_ret <= 0) {
                    // Klient sa odpojil
                    if (client_connected) {
                        client_connected = false;
                        client_disconnect_time = time(NULL);
                        printf("[SERVER DEBUG] Klient sa odpojil, čakam 10 sekúnd...\n");
                        game.menuSelect = '0';
                    }
                }
            }
            
            // Kontrola timeout po odpojení klienta
            if (!client_connected) {
                time_t now = time(NULL);
                if (difftime(now, client_disconnect_time) >= 10) {
                    printf("[SERVER DEBUG] Timeout po 10 sekundách bez klienta, vypinam server\n");
                    break;
                }
            }

            //printf("[SERVER DEBUG] received menuSelect='%c' paused=%d inGame=%d\n", game.menuSelect, game.paused, game.inGame);
            if (game.menuSelect == '1'){
                receive_menu(client, &game);
                init_game(&game);
                game.initTIme = time(NULL);
                game.gameCreate = true;
                memset(buf, 0, sizeof(buf));
                
            } else if (game.menuSelect == '2' && game.gameCreate == true){
                game.inGame = true;
                printf("[SERVER DEBUG] start new connection: inGame=%d paused=%d length=%d\n", game.inGame, game.paused, game.length);
                if (client_connected) send_state(client, &game);
            } else if (game.menuSelect == '3'){
                printf("[SERVER DEBUG] resume requested (paused=%d)\n", game.paused);
                if (game.paused) {
                    time_t now = time(NULL);
                    game.pausedSeconds += (int)(now - game.pauseStart);
                    game.paused = true;
                    game.inGame = true;
                    printf("[SERVER DEBUG] resumed game: inGame=%d paused=%d\n", game.inGame, game.paused);
                    printf("[DEBUG] pauza trvala %d sekúnd, spolu pauzy=%d\n",
                        (int)(now - game.pauseStart),
                        game.pausedSeconds);
                }
            } else if (game.menuSelect == '4'){
                game.end = true;
                if(game.gameCreate == false){
                    break;
                }
            }
        } else {
            if(game.paused == true){
                sleep(3);
                game.paused = false;
            }

            int recv_game = recv(client, &input, 1, MSG_DONTWAIT);
            if (recv_game < 0 && errno != EAGAIN) {
                client_connected = false;
                client_disconnect_time = time(NULL);
                printf("[SERVER DEBUG] Klient sa odpojil počas hry\n");
            }
            if (recv_game > 0) {
                if (input == 'p') {
                    game.pauseStart = time(NULL);
                    game.paused = true;
                    game.inGame = false;
                    printf("[SERVER DEBUG] pausing game and sending PAUSE\n");
                    if (client_connected) send(client, "PAUSE\n", 6, 0); 
                    continue;
                }
                if (input == 'q') {
                    reset_game(&game);
                    if (client_connected) send(client, "QUIT\n", 5, 0);
                    printf("[SERVER DEBUG] quting game and sending PAUSE\n");
                    continue;
                }
                if (input == 'w' || input == 'a' || input == 's' || input == 'd')
                    game.dir = input;
            }
            move_snake(&game);
            if(checkSelfCollision(&game)) {
                if (client_connected) send(client, "GAMEOVER\n", 9, 0);
                reset_game(&game);
                //game.inGame = false;
                //continue;
            }
            if (game.world == '2' && checkObjectCollision(&game)) {
                if (client_connected) send(client, "GAMEOVER\n", 9, 0);
                reset_game(&game);
            }
            if (game.mode == '2') {
                time_t now = time(NULL);
                int elapsed = (int)(now - game.initTIme) - game.pausedSeconds;

                printf("[SERVER DEBUG] čas: %d / %d sekúnd\n",
                    elapsed, game.maxTime);

                if (elapsed >= game.maxTime) {
                    if(game.end == true){
                        break;
                    }
                    send(client, "TIMEOVER\n", 9, 0);
                    reset_game(&game);
                    continue;
                }
            }
            if (game.snake[0].x == game.food_x &&
                game.snake[0].y == game.food_y) {
                game.length++;
                generate_food(&game);
            }

            send_state(client, &game);
            usleep(500000);
            
        }
    }

    if (game.objects) {
        free(game.objects);
        game.objects = NULL;
        game.objectCount = 0;
    }

    close(client);
    close(server_fd);
    printf("Server ukončený\n");
    return 0;
}
