#include "interrupts.h"
#include "video.h"
#include "game.h"


#define PADDLE_LENGTH 3
#define WELCOME_X 25
#define WELCOME_Y 12

void movePaddle(int x, int y, Player *p) {
    int i;

    // Remove previous paddle position
    for(i = 0; i < PADDLE_LENGTH; i++) {
        clearPixel(p->paddle_x, p->paddle_y + i);
    }

    // Draw new paddle
    for(i = 0; i < PADDLE_LENGTH; i++) {
        setPixel(x, y + i, GREEN, (char) 186);
    }

    p->paddle_x = x;
    p->paddle_y = y;
}

void show_welcome_screen() {
    printString(WELCOME_X, WELCOME_Y, "Welcome to Pong!");
    printString(WELCOME_X, WELCOME_Y + 1, "Player 1 press E and D to move."); 
    printString(WELCOME_X, WELCOME_Y + 2, "Player 2 press I and K to move."); 
    printString(WELCOME_X, WELCOME_Y + 3, "Press space to start playing!");

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

    init_interrupts();
    init_timer();
    init_keyboard();
    enable_interrupts();
     
    movePaddle(1, 20, &player1);
    movePaddle(78, 20, &player2);

    /* Loop forever, so that we don't fall back into the bootloader code. */
    while (1) {}
}


