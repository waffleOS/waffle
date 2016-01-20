#include "interrupts.h"
#include "video.h"
#include "game.h"


#define PADDLE_LENGTH 3
#define PADDLE_CHAR 186
#define BALL_CHAR 254
#define WELCOME_X 25
#define WELCOME_Y 12
#define WELCOME_WIDTH 35 
#define WELCOME_HEIGHT 5
#define SCORE_Y 9
#define MAX_SCORE 10

void initBall(int player_num) { 
    switch (player_num) {
        case 0: 
            pong_ball.x = 30;
            pong_ball.y = 20;
            pong_ball.v_x = -1;
            pong_ball.v_y = 1;
            break;
        case 1:
        default:
            pong_ball.x = 50;
            pong_ball.y = 20;
            pong_ball.v_x = 1;
            pong_ball.v_y = -1; 
            break;
    }

}

void initPlayers() { 
    int i;
    for (i = 0; i < NUM_PLAYERS; i++) {
        players[i].score = 0;
    }
    movePaddle(1, 20, players);
    movePaddle(78, 20, players + 1);

}

void clear_welcome_screen() { 
    int i, j;
    for (i = 0; i < WELCOME_WIDTH; i++) { 
        for (j = 0; j < WELCOME_HEIGHT; j++) {
            clearPixel(WELCOME_X + i, WELCOME_Y + j);
        }
    }
}

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

void movePaddle(int x, int y, Player *p) {
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

void show_welcome_screen() {
    printString(WELCOME_X, WELCOME_Y, "Welcome to Pong!");
    printString(WELCOME_X, WELCOME_Y + 1, "Player 0 press E and D to move."); 
    printString(WELCOME_X, WELCOME_Y + 2, "Player 1 press I and K to move."); 
    printString(WELCOME_X, WELCOME_Y + 3, "Press space to start playing!");

}



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

void printScores() { 
    setPixel(30, SCORE_Y, WHITE, '0' + players[0].score);
    setPixel(50, SCORE_Y, WHITE, '0' + players[1].score);
}

/* This is the entry-point for the game! */
void c_start(void) {
    /* TODO:  You will need to initialize various subsystems here.  This
     *        would include the interrupt handling mechanism, and the various
     *        systems that use interrupts.  Once this is done, you can call
     *        enable_interrupts() to start interrupt handling, and go on to
     *        do whatever else you decide to do!
     */
    init_video();
    setBackground(BLACK);
    int i;
    for (i = 0; i < SCREEN_HEIGHT; i++)
    {
        setPixel(0,i,GREEN,'0' + i % 10);
    }
    for (i = 0; i < SCREEN_WIDTH; i++)
    {
        setPixel(i,0,GREEN,'0' + i % 10);
    }

    setBackground(RED);
    show_welcome_screen();
    
    initPlayers();
    initBall(0);

    init_interrupts();
    init_timer();
    init_keyboard();
    enable_interrupts();
     
    
    /* Loop forever, so that we don't fall back into the bootloader code. */
    while (1) {
        printScores();
    }
}


