/**
 * game.h: Game data structures and global variables.
 */
#ifndef GAME_H
#define GAME_H 

#define TRUE  1
#define FALSE 0

#define NUM_PLAYERS 2

typedef enum {WELCOME, PLAY, END} game_state;

typedef struct Player {
    int paddle_x;
    int paddle_y;
    int score;
} Player;

typedef struct {
    int x;
    int y;
    int v_x;
    int v_y;
} Ball;

/* Global variables. */
game_state pong_state = WELCOME;
Player players[NUM_PLAYERS];
int playerSpeeds[NUM_PLAYERS];
Ball pong_ball;


/* Mutator functions. */
void initBall(int player_num);
void updateBall();
void movePaddle(int x, int y, Player *p);
void updateGame(void);
void getKey(void);

/* Check and handle all collisions. */
void handleCollisions();

#endif /* GAME_H */
