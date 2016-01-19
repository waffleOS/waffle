#include "interrupts.h"
#include "video.h"

typedef struct Player {
    int paddle_x;
    int paddle_y;
    int score;
} Player;

#define PADDLE_LENGTH 3

movePaddle(int x, int y, Player *p) {
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

void welcome_screen() {
    printString(20, 10, "Welcome to Pong!");

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
     welcome_screen();

     init_interrupts();
     init_timer();
     init_keyboard();
     enable_interrupts();
     
     Player p1;
     movePaddle(10, 20, &p1);
    /* Loop forever, so that we don't fall back into the bootloader code. */
    while (1) {}
}


