#include "hra.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>


void generate_food(Game *g) {
    bool valid;

    do {
        valid = true;

        g->food_x = rand() % (g->width - 2) + 1;
        g->food_y = rand() % (g->height - 2) + 1;

        for (int i = 0; i < g->length; i++) {
            if (g->food_x == g->snake[i].x &&
                g->food_y == g->snake[i].y) {
                valid = false;
                break;
            }
        }

        if (g->world == '2') {
            for (int i = 0; i < g->objectCount; i++) {
                if (g->food_x == g->objects[i].x &&
                    g->food_y == g->objects[i].y) {
                    valid = false;
                    break;
                }
            }
        }

    } while (!valid);
}

void init_game(Game *g) {
    g->length = 1;
    g->snake[0].x = g->width / 2;
    g->snake[0].y = g->height / 2;
    g->dir = 'd';
    g->menuSelect = '0';
    g->inGame = false;
    g->gameCreate = false;
    g->end = false;
    generate_food(g);
    g->initTIme = time(NULL);
    g->clientDisconnect = 0;
    if(g->world == '2'){
        g->objectCount = (int)sqrt((g->width * g->height)/2);
        g->objects = malloc(sizeof(Coordinates) * g->objectCount);
        if (!g->objects) {
            perror("malloc failed");
            exit(1);
        }
        for (int i = 0; i < g->objectCount; i++) {
        bool valid;
        do {
            valid = true;
            g->objects[i].x = rand() % (g->width - 2) + 1;
            g->objects[i].y = rand() % (g->height - 2) + 1;

            if (g->objects[i].x == g->food_x &&
                g->objects[i].y == g->food_y) {
                valid = false;
            }

            for (int j = 0; j < g->length; j++) {
                if (g->objects[i].x == g->snake[j].x &&
                    g->objects[i].y == g->snake[j].y) {
                    valid = false;
                    break;
                }
            }

            for (int j = 0; j < i; j++) {
                if (g->objects[i].x == g->objects[j].x &&
                    g->objects[i].y == g->objects[j].y) {
                    valid = false;
                    break;
                }
            }

        } while (!valid);
    }
    } else {
        g->objects = NULL;
        g->objectCount = 0;
    }
}

void checkDirection(Game *g) {
    if (
    (g->lastDir == 'w' && g->dir == 's') || (g->lastDir == 's' && g->dir == 'w') ||
    (g->lastDir == 'a' && g->dir == 'd') ||(g->lastDir == 'd' && g->dir == 'a')) {
        g->dir = g->lastDir;
    }
    
}

bool checkSelfCollision(Game *g) {
    for (int i = 1; i < g->length; i++) {
        if (g->snake[0].x == g->snake[i].x && g->snake[0].y == g->snake[i].y) {
            g->length = i;
            return true;
        }
    }
    return false;
}

bool checkObjectCollision(Game *g){
    for (int i = 0; i < g->objectCount; i++) {
        if (g->snake[0].x == g->objects[i].x &&
            g->snake[0].y == g->objects[i].y) {
            return true;
        }
    }
    return false;
}

void outOfWorld(Game *g) {
    if (g->snake[0].x < 1){
        g->snake[0].x = g->width - 2;
    }
    else if (g->snake[0].x >= g->width - 1){
        g->snake[0].x = 1;
    }

    if (g->snake[0].y < 1){
        g->snake[0].y = g->height - 2;
    }
    else if (g->snake[0].y >= g->height - 1){
        g->snake[0].y = 1;
    }
}

void reset_game(Game *g) {
    g->length = 0;
    g->dir = 'd';
    g->inGame = false;
    g->gameCreate = false;
    g->lastDir = 'd';
    g->menuSelect = '0';
    g->initTIme = time(NULL);
    g->clientDisconnect = 0;

    if (g->objects) {
        free(g->objects);
        g->objects = NULL;
        g->objectCount = 0;
    }
}

void move_snake(Game *g) {

    checkDirection(g);

    for (int i = g->length - 1; i > 0; i--) {
        g->snake[i] = g->snake[i - 1];
    }
    g->lastDir = g->dir;
    switch (g->dir) {
        case 'w':   
            g->snake[0].y--;
            break;
        case 's':  
            g->snake[0].y++;
            break;
        case 'a':  
            g->snake[0].x--;
            break;
        case 'd': 
            g->snake[0].x++;
            break;
    }

    outOfWorld(g);

}

void send_state(int client, Game *g) {
    char buf[BUFFER];
    int off = 0;
    time_t now = time(NULL);
    int time = (int)difftime(now, g->initTIme);
    off += snprintf(buf + off, BUFFER - off,
        "SIZE %d %d\nFOOD %d %d\nLEN %d\nTIME %d\nOBJ %d\n",
        g->width, g->height, g->food_x, g->food_y, g->length, time, g->objectCount);

    for (int i = 0; i < g->length; i++) {
        off += snprintf(buf + off, BUFFER - off,
        "S %d %d\n", g->snake[i].x, g->snake[i].y);

    }

    if (g->world == '2' && g->objects && g->objectCount > 0) {
        for (int i = 0; i < g->objectCount; i++) {
            off += snprintf(buf + off, BUFFER - off,
                "O %d %d\n",
                g->objects[i].x,
                g->objects[i].y
            );
        }
    }

    off += snprintf(buf + off, BUFFER - off, "END\n");
    send(client, buf, off, 0);
}

void receive_menu(int client, Game *game) {
    char buf[BUFFER];
    int n = recv(client, buf, BUFFER - 1, 0);
    if (n <= 0) return;
    buf[n] = '\0';

    char *line = strtok(buf, "\n");
    while (line) {
        sscanf(line, "WIDTH=%d", &game->width);
        sscanf(line, "HEIGHT=%d", &game->height);
        sscanf(line, "WORLD=%c", &game->world);
        sscanf(line, "MODE=%c", &game->mode);
        sscanf(line, "TIME=%d", &game->maxTime);
        line = strtok(NULL, "\n");
    }
}
