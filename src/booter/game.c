#include "interrupts.h"
#include "video.h"
#include "game.h"
#include "keyboard.h"

#define PADDLE_LENGTH 3
#define PADDLE_CHAR 186
#define BALL_CHAR 254
#define WELCOME_X 25
#define WELCOME_Y 12
#define WELCOME_WIDTH 35
#define WELCOME_HEIGHT 5
#define SCORE_Y 9
#define MAX_SCORE 10
/* Speed up interval in deciseconds */
#define BALL_SPEED_UP_INTERVAL 500
/* Speed value is inverse of actual speed. ie low values mean high speed */
#define BALL_SPEED_START 10;

unsigned int time_count;

/**
 * Initials the ball.
 * player_num indicates, which side / direction it should
 * start on (0 for left and 1 for right player).
 */
void initBall(int player_num) {
    clearPixel(pong_ball.x, pong_ball.y);

    switch (player_num) {
        case 0:
            pong_ball.x = 30;
            pong_ball.y = time_count % 15 + 5;
            pong_ball.v_x = -1;
            pong_ball.v_y = 1;
            pong_ball.ball_speed = BALL_SPEED_START;
            pong_ball.exists = 1;
            break;
        case 1:
        default:
            pong_ball.x = 50;
            pong_ball.y = time_count % 15 + 5;
            pong_ball.v_x = 1;
            pong_ball.v_y = -1;
            pong_ball.ball_speed = BALL_SPEED_START;
            pong_ball.exists = 1;
            break;
    }

}

/* Initializes the paddles and scores. */
void initPlayers() {
    int i;
    for (i = 0; i < NUM_PLAYERS; i++) {
        players[i].score = 0;
        players[i].paddle_vy_up = 0;
        players[i].paddle_vy_down = 0;
    }
    movePaddle(1, 20, players);
    movePaddle(78, 20, players + 1);

}

/**
 * Clears the welcome_screen area, which is used for
 * messages to the user (starts at WELCOME_X, WELCOME_Y).
 */
void clear_welcome_screen() {
    int i, j;
    for (i = 0; i < WELCOME_WIDTH; i++) {
        for (j = 0; j < WELCOME_HEIGHT; j++) {
            clearPixel(WELCOME_X + i, WELCOME_Y + j);
        }
    }
}

/**
 * Updates the ball's position and accounts for walls
 * and scoring.
 */
void updateBall() {
    clearPixel(pong_ball.x, pong_ball.y);
    pong_ball.x += pong_ball.v_x;
    if (pong_ball.x < 0) {
        if (++players[1].score == MAX_SCORE) {
            clear_welcome_screen();
            printString(WELCOME_X, WELCOME_Y, "Player 1 wins!");
            pong_state = END;
            initPlayers();
        }
        initBall(1);
    }
    else if (pong_ball.x >= SCREEN_WIDTH) {
        if (++players[0].score == MAX_SCORE) {
            clear_welcome_screen();
            printString(WELCOME_X, WELCOME_Y, "Player 0 wins!");
            pong_state = END;
            initPlayers();
        }
        initBall(0);
    }
    pong_ball.y += pong_ball.v_y;
    if (pong_ball.y < 0) {
        pong_ball.y = 0;
        pong_ball.v_y *= -1;
    }
    else if (pong_ball.y >= SCREEN_HEIGHT) {
        pong_ball.y = SCREEN_HEIGHT - 1;
        pong_ball.v_y *= -1;
    }
    setPixel(pong_ball.x, pong_ball.y, WHITE, (char) BALL_CHAR);
}

void updatePlayers(void)
{
    int i;
    for (i = 0; i < NUM_PLAYERS; i++)
    {
        movePaddle(players[i].paddle_x, players[i].paddle_y - players[i].paddle_vy_up + players[i].paddle_vy_down, &(players[i]));
    }
}

/* Moves the paddle of a given player. */
void movePaddle(int x, int y, Player *p) {

    if (y < 0 || y + PADDLE_LENGTH > SCREEN_HEIGHT)
    {
        return;
    }
    int i;

    // Remove previous paddle position
    for(i = 0; i < PADDLE_LENGTH; i++) {
        clearPixel(p->paddle_x, p->paddle_y + i);
    }

    // Draw new paddle
    for(i = 0; i < PADDLE_LENGTH; i++) {
        setPixel(x, y + i, GREEN, (char) PADDLE_CHAR);
    }

    p->paddle_x = x;
    p->paddle_y = y;
}

/* Prints welcome screen with instructions to the player. */
void show_welcome_screen() {
    printString(WELCOME_X, WELCOME_Y, "Welcome to Pong!");
    printString(WELCOME_X, WELCOME_Y + 1, "Player 0 press E and D to move.");
    printString(WELCOME_X, WELCOME_Y + 2, "Player 1 press I and K to move.");
    printString(WELCOME_X, WELCOME_Y + 3, "Press space to start playing!");

}

/* Handles collisions between the ball and the paddles. */
void handleCollisions() {
    int i;
    for (i = 0; i < NUM_PLAYERS; i++) {
        if (pong_ball.x == players[i].paddle_x &&
            pong_ball.y >= players[i].paddle_y &&
            pong_ball.y < players[i].paddle_y + PADDLE_LENGTH) {
            setPixel(pong_ball.x, pong_ball.y, GREEN, (char) PADDLE_CHAR);
            pong_ball.v_x *= -1;
            pong_ball.x += pong_ball.v_x;
        }
    }

}

/* Prints the scores of the players. */
void printScores() {
    setPixel(30, SCORE_Y, WHITE, '0' + players[0].score);
    setPixel(50, SCORE_Y, WHITE, '0' + players[1].score);
}

void updateGame(void)
{
    getKey();
}

void getKey(void)
{
    unsigned char scan_code;
    int keyPressed = dequeue(&scan_code);
    if (keyPressed)
    {
        unsigned int scanCodeValue = (unsigned int) scan_code;
        switch (scanCodeValue)
        {
            case E_DOWN:
                players[0].paddle_vy_up = 1;
                break;
            case E_UP:
                players[0].paddle_vy_up = 0;
                break;
            case D_DOWN:
                players[0].paddle_vy_down = 1;
                break;
            case D_UP:
                players[0].paddle_vy_down = 0;
                break;
            case I_DOWN:
                players[1].paddle_vy_up = 1;
                break;
            case I_UP:
                players[1].paddle_vy_up = 0;
                break;
            case K_DOWN:
                players[1].paddle_vy_down = 1;
                break;
            case K_UP:
                players[1].paddle_vy_down = 0;
                break;
            case SPACE_DOWN:
                switch(pong_state)
                {
                    case WELCOME:
                        clear_welcome_screen();
                        pong_state = PLAY;
                    case END:
                        initBall(0);
                        break;
                    case PLAY:
                        pong_state = PAUSE;
                        break;
                    case PAUSE:
                        pong_state = PLAY;
                        break;
                }
                break;
        }
    }
}

/* Steps the game forward one time step. */
void stepGame() {
    time_count++;
    if(time_count % 5 == 0) {
        updatePlayers();
    }
    if (pong_ball.ball_speed > 0 && time_count % pong_ball.ball_speed == 0 &&
                                        pong_state != PAUSE) {
        updateBall();
        handleCollisions();
    }

    if(pong_ball.exists && pong_state != PAUSE && pong_ball.ball_speed > 0 &&
                                time_count % BALL_SPEED_UP_INTERVAL == 0) {
        pong_ball.ball_speed--;
    }
}


/* This is the entry-point for the game! */
void c_start(void) {
    /* Set up video module. */
    init_video();
    setBackground(BROWN);
    show_welcome_screen();

    /* Add players and balls. */
    initPlayers();
    pong_ball.x = 30;
    pong_ball.y = 20;
    pong_ball.v_x = 0;
    pong_ball.v_y = 0;

    /* Set up and enable interrupts. */
    init_interrupts();
    init_timer();
    init_keyboard();
    enable_interrupts();

    time_count = 0;
    pong_ball.exists = 0;

    /* Game loop. */
    while (1) {
        printScores();
        updateGame();
    }
}
