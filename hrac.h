#ifndef HRAC_H
#define HRAC_H

#include <stdbool.h>
#include <pthread.h>
#include <errno.h>

#define PORT 13777
#define BUFFER 2048
#define MAX_SNAKE 1000




typedef struct {
    int x, y;
} Segment;

typedef enum {
    GAME_STATE_GAMENOCREATE,
    GAME_STATE_GAMECREATE,
    GAME_STATE_PLAYING,
    GAME_STATE_CANCEL
} GameState;

typedef struct {
    char mode;
    char world;
    char menuSelect;

    int width;
    int height;
    int food_x;
    int food_y;
    int length;
    int time;
    int gameTime;
    int obj;
    bool showMenu;

    Segment snake[MAX_SNAKE];
    Segment *objects;

    GameState gameStat;

    int socket_fd;
    int running;
    pthread_mutex_t lock;
} Game;

void raw_on();
void raw_off();

void show_menu(Game *game, char buf[]);
void initGame(Game *game);
void printGame(Game *game);
void readData(Game *game, char buf[]);

#endif
