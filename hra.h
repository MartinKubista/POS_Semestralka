#ifndef HRA_H
#define HRA_H

#include <stdbool.h>
#include <time.h>

#define PORT 13777
#define BUFFER 2048
#define MAX_SNAKE 1000

typedef struct {
    int x, y;
} Coordinates;

typedef struct {
    int width, height;
    int food_x, food_y;
    int length;
    Coordinates snake[MAX_SNAKE];
    Coordinates *objects;
    int objectCount;
    char dir;
    char lastDir;
    char mode, world;
    char menuSelect;
    int maxTime;
    bool inGame;
    bool gameCreate;
    bool paused;
    bool end;
    time_t pauseStart;
    int pausedSeconds;
    time_t initTIme;
    int clientDisconnect;
} Game;

void init_game(Game *g);
void reset_game(Game *g);
void generate_food(Game *g);
void move_snake(Game *g);
void send_state(int client, Game *g);
void receive_menu(int client, Game *g);

bool checkSelfCollision(Game *g);
bool checkObjectCollision(Game *g);

#endif
