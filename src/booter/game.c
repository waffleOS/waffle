#include "video.h"
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
         setPixel(0,i,GREEN,'0' + i);
     }
     for (i = 0; i < SCREEN_WIDTH; i++)
     {
         setPixel(i,0,GREEN,'0' + i);
     }

     setBackground(RED);
     //setPixel(0,0,GREEN,'0');
     //setPixel(10,10,BLUE,'i');

    /* Loop forever, so that we don't fall back into the bootloader code. */
    while (1) {}
}
