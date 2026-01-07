#include "hrac.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <termios.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>

void raw_on(Game *game) {
    struct termios newt;
    tcgetattr(STDIN_FILENO, &game->oldt);
    newt = game->oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); 
    game->oldf = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, game->oldf | O_NONBLOCK);
}

void raw_off(Game *game) {
    tcsetattr(STDIN_FILENO, TCSANOW, &game->oldt);
    fcntl(STDIN_FILENO, F_SETFL, game->oldf);
}

int read_int_range(const char *prompt, int min, int max) {
    char buf[BUFFER];
    long val;
    char *end;

    while (1) {
        printf("%s (%d - %d): ", prompt, min, max);

        if (!fgets(buf, sizeof(buf), stdin)) {
            printf("Chyba vstupu\n");
            continue;
        }

        errno = 0;
        val = strtol(buf, &end, 10);

        if (errno != 0 || end == buf || (*end != '\n' && *end != '\0')) {
            printf("Zadaj platne cislo.\n");
            continue;
        }

        if (val < min || val > max) {
            printf("Hodnota musi byt v rozsahu %d - %d.\n", min, max);
            continue;
        }

        return (int)val;
    }
}

void show_menu(Game *game, char buf[]) {
    printf("Stlacte 1: Pre zacatie novej hry\n");
    if (game->gameStat == GAME_STATE_GAMECREATE) {
        printf("Stlacte 2: Pre pripojenie sa do hry\n");
    }
    printf("Stlacte 3: Pre ukoncenie\n");

    fgets(buf, BUFFER, stdin);
    game->menuSelect = buf[0];

    if (game->menuSelect == '1') {

        do {
            printf("Stlacte 1: Standardny rezim\n");
            printf("Stlacte 2: Casovy rezim\n");
            fgets(buf, BUFFER, stdin);
            game->mode = buf[0];
        } while (game->mode != '1' && game->mode != '2');

        do {
            printf("Stlacte 1: Svet bez prekazok\n");
            printf("Stlacte 2: Svet s prekazkami\n");
            fgets(buf, BUFFER, stdin);
            game->world = buf[0];
        } while (game->world != '1' && game->world != '2');

        game->width  = read_int_range("Zadaj sirku sveta", 4, 15) + 2;
        game->height = read_int_range("Zadaj vysku sveta", 4, 15) + 2;

        if (game->mode == '2') {
            game->time = read_int_range("Zadaj cas hry (sekundy)", 20, 180);
        }
    }
}


void initGame(Game *game) {
    game->mode = '0';
    game->world = '0';
    game->menuSelect = '0';

    game->width = 0;
    game->height = 0;
    game->food_x = 0;
    game->food_y = 0;
    game->length = 0;
    game->time = 0;
    game->obj = 0;
    game->showMenu = true;
    game->gameTime = 0;

    game->objects = NULL;

    game->gameStat = GAME_STATE_GAMENOCREATE;
}

void printGame(Game *game){
    system("clear");
        printf("Skore: %d\n", game->length);
        printf("Cas hry: %d\n", game->gameTime);
        for (int y = 0; y < game->height; y++) {
            for (int x = 0; x < game->width; x++) {
                if (x==0||y==0||x==game->width-1||y==game->height-1) printf("#");
                else if (x==game->food_x && y==game->food_y) printf("*");
                else {
                    int p=0;
                    if (game->world == '2')
                    {
                        for (int i = 0; i < game->obj; i++) {
                        if (game->objects[i].x == x && game->objects[i].y == y) {
                            printf("X");  
                            p = 1;
                            break;
                        }
                    }
                    }
                    
                    for (int j=0;j<game->length;j++)
                        if (game->snake[j].x==x && game->snake[j].y==y) {
                            printf(j==0?"O":"o");
                            p=1; 
                            break;
                        }
                    if (!p) printf(" ");
                }
            }
            printf("\n");
        }
}

void readData(Game *game, char buf[]){
    char *line = strtok(buf, "\n");
    int snake_i = 0;
    int obj_i = 0;
    while (line) {
        if (sscanf(line, "SIZE %d %d", &game->width, &game->height)  == 2){

        } else if (sscanf(line, "FOOD %d %d", &game->food_x, &game->food_y)  == 2){

        } else if (sscanf(line, "LEN %d", &game->length)  == 1){

        }else if (sscanf(line, "TIME %d", &game->gameTime)  == 1){

        } else if (sscanf(line, "OBJ %d", &game->obj)  == 1){
            if (game->objects) {
              free(game->objects);
              game->objects = NULL;
            }
            if(game->obj != 0){
              game->objects = malloc(sizeof(Segment) * game->obj);
              obj_i = 0;
            }
            
        } else if (line[0] == 'S'){
            sscanf(line, "S %d %d", &game->snake[snake_i].x, &game->snake[snake_i].y);
            snake_i++;
        }else if (line[0] == 'O') {
            sscanf(line, "O %d %d", &game->objects[obj_i].x, &game->objects[obj_i].y);
            obj_i++;
        } else if (strcmp(line, "GAMEOVER") == 0) {
            raw_off(game);
            game->running = 0;
            if (game->objects) {        
              free(game->objects);
              game->objects = NULL;
              game->obj = 0;
            }
            game->showMenu = true;
            game->gameStat = GAME_STATE_GAMENOCREATE;
            printf("GAME OVER\n");
            printf("Pre pokracovanie stlacte ENTER\n");
        } else if (strcmp(line, "QUIT") == 0) {
            raw_off(game);
            game->running = 0;
            if (game->objects) {          
              free(game->objects);
              game->objects = NULL;
            }
            game->showMenu = true;
            game->gameStat = GAME_STATE_GAMENOCREATE;
            printf("QUIT\n");
            printf("Pre pokracovanie stlacte ENTER\n");
        } else if (strcmp(line, "TIMEOVER") == 0) {
            raw_off(game);
            game->running = 0;
            game->showMenu = true;
            game->gameStat = GAME_STATE_GAMENOCREATE;
            system("clear");
            printf("TIMEOVER\n");
            printf("Pre pokracovanie stlacte ENTER\n");
        }
        line = strtok(NULL, "\n");
    }
}
