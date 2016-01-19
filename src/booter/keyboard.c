#include "ports.h"

/* This is the IO port of the PS/2 controller, where the keyboard's scan
 * codes are made available.  Scan codes can be read as follows:
 *
 *     unsigned char scan_code = inb(KEYBOARD_PORT);
 *
 * Most keys generate a scan-code when they are pressed, and a second scan-
 * code when the same key is released.  For such keys, the only difference
 * between the "pressed" and "released" scan-codes is that the top bit is
 * cleared in the "pressed" scan-code, and it is set in the "released" scan-
 * code.
 *
 * A few keys generate two scan-codes when they are pressed, and then two
 * more scan-codes when they are released.  For example, the arrow keys (the
 * ones that aren't part of the numeric keypad) will usually generate two
 * scan-codes for press or release.  In these cases, the keyboard controller
 * fires two interrupts, so you don't have to do anything special - the
 * interrupt handler will receive each byte in a separate invocation of the
 * handler.
 *
 * See http://wiki.osdev.org/PS/2_Keyboard for details.
 */
#define KEYBOARD_PORT 0x60
#define QUEUE_SIZE 1024



/* TODO:  You can create static variables here to hold keyboard state.
 *        Note that if you create some kind of circular queue (a very good
 *        idea, you should declare it "volatile" so that the compiler knows
 *        that it can be changed by exceptional control flow.
 *
 *        Also, don't forget that interrupts can interrupt *any* code,
 *        including code that fetches key data!  If you are manipulating a
 *        shared data structure that is also manipulated from an interrupt
 *        handler, you might want to disable interrupts while you access it,
 *        so that nothing gets mangled...
 */

 typedef struct Queue {
     int headIndex;
     int tailIndex;
     char values[QUEUE_SIZE];
     int empty;
     int full;
 } Queue;

 void initializeQueue(Queue * q);



 Queue q;


void init_keyboard(void) {
    /* TODO:  Initialize any state required by the keyboard handler. */

    initializeQueue(&q);

    /* TODO:  You might want to install your keyboard interrupt handler
     *        here as well.
     */
}

void initializeQueue(Queue * q)
{
    q->headIndex = 0;
    q->tailIndex = 0;
    q->empty = 1;
    q->full = 0;
}

int isEmpty(Queue * q)
{
    return q->empty;
}

int isFull(Queue * q)
{
    return q->full;
}

void enqueue(Queue * q, char scan_code)
{
    while (isFull(q))
    {

    }

    q->values[q->tailIndex++] = scan_code;
    if (q->tailIndex == q->headIndex)
    {
        q->full = 1;
    }
}

char dequeue(Queue * q)
{
    while (isEmpty(q))
    {

    }

    char scan_code = q->values[q->headIndex++];
    if (q->headIndex == q->tailIndex)
    {
        q->empty = 1;
    }

    return scan_code;
}

void keyboardHandler()
{
    unsigned char scan_code = inb(KEYBOARD_PORT);
    enqueue(&q, scan_code);
}
