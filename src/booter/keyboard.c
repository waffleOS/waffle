#include "ports.h"
#include "handlers.h"
#include "interrupts.h"
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



 typedef struct Queue {
     int headIndex;
     int tailIndex;
     char values[QUEUE_SIZE];
     int empty;
     int full;
 } Queue;

 void initializeQueue(volatile Queue * q);



 Queue volatile q;


void init_keyboard(void) {
    initializeQueue(&q);

    install_interrupt_handler(KEYBOARD_INTERRUPT, keyboard_handler);
}

void initializeQueue(volatile Queue * q)
{
    q->headIndex = 0;
    q->tailIndex = 0;
    q->empty = 1;
    q->full = 0;
}

int isEmpty(volatile Queue * q)
{
    return q->empty;
}

int isFull(volatile Queue * q)
{
    return q->full;
}

void enqueue_q(volatile Queue * q, char scan_code)
{
    if (isFull(q))
    {
        return;
    }


    q->values[q->tailIndex++] = scan_code;
    q->tailIndex %= QUEUE_SIZE;
    if (q->tailIndex == q->headIndex)
    {
        q->full = 1;
    }
    q->empty = 0;
}

int dequeue_q(volatile Queue * q, char * scan_code)
{
    if (isEmpty(q))
    {
        return 0;
    }

    disable_interrupts();
    *scan_code = q->values[q->headIndex++];
    q->headIndex %= QUEUE_SIZE;
    if (q->headIndex == q->tailIndex)
    {
        q->empty = 1;
    }
    q->full = 0;
    enable_interrupts();
    return 1;
}

int dequeue(char * scan_code)
{
    return dequeue_q(&q, scan_code);
}

void keyboardHandler(void)
{
    unsigned char scan_code = inb(KEYBOARD_PORT);
    enqueue_q(&q, scan_code);
}
