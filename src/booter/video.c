#include "video.h"

/* This is the address of the VGA text-mode video buffer.  Note that this
 * buffer actually holds 8 pages of text, but only the first page (page 0)
 * will be displayed.
 *
 * Individual characters in text-mode VGA are represented as two adjacent
 * bytes:
 *     Byte 0 = the character value
 *     Byte 1 = the color of the character:  the high nibble is the background
 *              color, and the low nibble is the foreground color
 *
 * See http://wiki.osdev.org/Printing_to_Screen for more details.
 *
 * Also, if you decide to use a graphical video mode, the active video buffer
 * may reside at another address, and the data will definitely be in another
 * format.  It's a complicated topic.  If you are really intent on learning
 * more about this topic, go to http://wiki.osdev.org/Main_Page and look at
 * the VGA links in the "Video" section.
 */
#define VIDEO_BUFFER ((void *) 0xB8000)

#define SCREEN_HEIGHT 40
#define SCREEN_WIDTH 80
#define MAP_XY_TO_INDEX(x, y) (2 * (SCREEN_WIDTH * (y) + (x)))

/* TODO:  You can create static variables here to hold video display state,
 *        such as the current foreground and background color, a cursor
 *        position, or any other details you might want to keep track of!
 */


void init_video(void) {
    /* TODO:  Do any video display initialization you might want to do, such
     *        as clearing the screen, initializing static variable state, etc.
     */
     background_color = BLACK;
     setBackground(background_color);
}


void setPixel(int x, int y, char color, char value) {
	MAP_XY_TO_INDEX(x, y)
}

void clearForeground() {
	int i;
	for(i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH * 2; i += 2) {
		*(VIDEO_BUFFER + i) = ' ';
	}
}

void setBackground(char color) {

}

void clearPixel(int x, int y) {
	*(VIDEO_BUFFER + MAP_XY_TO_INDEX(x, y)) = ' ';
}