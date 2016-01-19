#ifndef VIDEO_H
#define VIDEO_H


/* Available colors from the 16-color palette used for EGA and VGA, and
 * also for text-mode VGA output.
 */
#define BLACK          0
#define BLUE           1
#define GREEN          2
#define CYAN           3
#define RED            4
#define MAGENTA        5
#define BROWN          6
#define LIGHT_GRAY     7
#define DARK_GRAY      8
#define LIGHT_BLUE     9
#define LIGHT_GREEN   10
#define LIGHT_CYAN    11
#define LIGHT_RED     12
#define LIGHT_MAGENTA 13
#define YELLOW        14
#define WHITE         15

#define SCREEN_HEIGHT 30
#define SCREEN_WIDTH 80


void init_video(void);
void setPixel(int x, int y, char color, char value);
void clearForeground(void);
void setBackground(char color);
void clearPixel(int x, int y);


#endif /* VIDEO_H */
