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

char background_color;

void init_video(void) {
    /* TODO:  Do any video display initialization you might want to do, such
     *        as clearing the screen, initializing static variable state, etc.
     */
     background_color = BLACK;
     setBackground(background_color);

     int i;
     for(i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH * 2; i += 2) {
     	*(char *) VIDEO_BUFFER + i) = ' ';
     }
}

/* Given an x y coordinate, a foreground color, and a value, sets that
 * coordinate in the video buffer.
 */
void setPixel(int x, int y, char color, char value) {
	int index = MAP_XY_TO_INDEX(x, y);
    *(VIDEO_BUFFER + index) = value;
    *(VIDEO_BUFFER + index + 1) = (background_color << 4) | color;
}

/* Clears all the foreground values in the video buffer leaving a blank screen
 * of just the background color
 */
void clearForeground() {
	int i;
	for(i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH * 2; i += 2) {
		*((char *)VIDEO_BUFFER + i) = ' ';
	}
}

/* Sets the background color of everything in the video buffer and redraws
 * all coordinates with this new background.
 */
void setBackground(char color) {
    background_color = color;
    int i;
    for (i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH * 2; i += 2)
    {
        char color = *(VIDEO_BUFFER + i + 1);
        *(VIDEO_BUFFER + i + 1) = ((color << 4) >> 4) | (color << 4);
    }

}

/* Given an x y coordinate, clears the value at the specified coordinate.
 */
void clearPixel(int x, int y) {
	*((char *)VIDEO_BUFFER + MAP_XY_TO_INDEX(x, y)) = ' ';
}
