/*! \file synch.c
 *
 * Implementation of various thread synchronization primitives.
 */

/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/*! Initializes semaphore SEMA to VALUE.  A semaphore is a
    nonnegative integer along with two atomic operators for
    manipulating it:

    - down or "P": wait for the value to become positive, then
      decrement it.

    - up or "V": increment the value (and wake up one waiting
      thread, if any). */
void sema_init(struct semaphore *sema, unsigned value) {
    ASSERT(sema != NULL);

    sema->value = value;
    list_init(&sema->waiters);
}

/*! Down or "P" operation on a semaphore.  Waits for SEMA's value
    to become positive and then atomically decrements it.

    This function may sleep, so it must not be called within an
    interrupt handler.  This function may be called with
    interrupts disabled, but if it sleeps then the next scheduled
    thread will probably turn interrupts back on. */
void sema_down(struct semaphore *sema) {
    enum intr_level old_level;

    ASSERT(sema != NULL);
    ASSERT(!intr_context());

    old_level = intr_disable();
    while (sema->value == 0) {
        list_push_back(&sema->waiters, &thread_current()->elem);
        thread_block();
    }
    sema->value--;
    intr_set_level(old_level);
}

/*! Down or "P" operation on a semaphore, but only if the
    semaphore is not already 0.  Returns true if the semaphore is
    decremented, false otherwise.

    This function may be called from an interrupt handler. */
bool sema_try_down(struct semaphore *sema) {
    enum intr_level old_level;
    bool success;

    ASSERT(sema != NULL);

    old_level = intr_disable();
    if (sema->value > 0) {
        sema->value--;
        success = true; 
    }
    else {
      success = false;
    }
    intr_set_level(old_level);

    return success;
}

/*! Up or "V" operation on a semaphore.  Increments SEMA's value
    and wakes up one thread of those waiting for SEMA, if any.

    This function may be called from an interrupt handler. */
void sema_up(struct semaphore *sema) {
    enum intr_level old_level;

    ASSERT(sema != NULL);

    old_level = intr_disable();
    if (!list_empty(&sema->waiters)) {
        printf("%d threads waiting on sema\n", list_size(&sema->waiters));
        thread_unblock(list_entry(list_pop_front(&sema->waiters),
                                  struct thread, elem));
    }
    sema->value++;
    intr_set_level(old_level);
}

static void sema_test_helper(void *sema_);

/*! Self-test for semaphores that makes control "ping-pong"
    between a pair of threads.  Insert calls to printf() to see
    what's going on. */
void sema_self_test(void) {
    struct semaphore sema[2];
    int i;

    printf("Testing semaphores...");
    sema_init(&sema[0], 0);
    sema_init(&sema[1], 0);
    thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
    for (i = 0; i < 10; i++) {
        sema_up(&sema[0]);
        sema_down(&sema[1]);
    }
    printf ("done.\n");
}

/*! Thread function used by sema_self_test(). */
static void sema_test_helper(void *sema_) {
    struct semaphore *sema = sema_;
    int i;

    for (i = 0; i < 10; i++) {
        sema_down(&sema[0]);
        sema_up(&sema[1]);
    }
}

/*! Initializes LOCK.  A lock can be held by at most a single
    thread at any given time.  Our locks are not "recursive", that
    is, it is an error for the thread currently holding a lock to
    try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void lock_init(struct lock *lock) {
    ASSERT(lock != NULL);

    lock->holder = NULL;
    sema_init(&lock->semaphore, 1);
}

/*! Acquires LOCK, sleeping until it becomes available if
    necessary.  The lock must not already be held by the current
    thread.

    This function may sleep, so it must not be called within an
    interrupt handler.  This function may be called with
    interrupts disabled, but interrupts will be turned back on if
    we need to sleep. */
void lock_acquire(struct lock *lock) {
    ASSERT(lock != NULL);
    ASSERT(!intr_context());
    ASSERT(!lock_held_by_current_thread(lock));

    //volatile struct thread *t = thread_current();
    //printf("Thread %s waiting for %p\n", t->name, lock); 
    sema_down(&lock->semaphore);
    struct thread *t = thread_current();
    lock->holder = t;
    printf("Thread %s Got %p\n", t->name, lock);
}

/*! Tries to acquires LOCK and returns true if successful or false
    on failure.  The lock must not already be held by the current
    thread.

    This function will not sleep, so it may be called within an
    interrupt handler. */
bool lock_try_acquire(struct lock *lock) {
    bool success;

    ASSERT(lock != NULL);
    ASSERT(!lock_held_by_current_thread(lock));

    success = sema_try_down(&lock->semaphore);
    if (success)
      lock->holder = thread_current();

    return success;
}

/*! Releases LOCK, which must be owned by the current thread.

    An interrupt handler cannot acquire a lock, so it does not
    make sense to try to release a lock within an interrupt
    handler. */
void lock_release(struct lock *lock) {
    ASSERT(lock != NULL);
    ASSERT(lock_held_by_current_thread(lock));

    printf("Releasing lock %s %p\n", lock->name, lock);
    lock->holder = NULL;
    sema_up(&lock->semaphore);
}

/*! Returns true if the current thread holds LOCK, false
    otherwise.  (Note that testing whether some other thread holds
    a lock would be racy.) */
bool lock_held_by_current_thread(const struct lock *lock) {
    ASSERT(lock != NULL);

    return lock->holder == thread_current();
}

/*! One semaphore in a list. */
struct semaphore_elem {
    struct list_elem elem;              /*!< List element. */
    struct semaphore semaphore;         /*!< This semaphore. */
};

/*! Initializes condition variable COND.  A condition variable
    allows one piece of code to signal a condition and cooperating
    code to receive the signal and act upon it. */
void cond_init(struct condition *cond) {
    ASSERT(cond != NULL);

    list_init(&cond->waiters);
}

/*! Atomically releases LOCK and waits for COND to be signaled by
    some other piece of code.  After COND is signaled, LOCK is
    reacquired before returning.  LOCK must be held before calling
    this function.

    The monitor implemented by this function is "Mesa" style, not
    "Hoare" style, that is, sending and receiving a signal are not
    an atomic operation.  Thus, typically the caller must recheck
    the condition after the wait completes and, if necessary, wait
    again.

    A given condition variable is associated with only a single
    lock, but one lock may be associated with any number of
    condition variables.  That is, there is a one-to-many mapping
    from locks to condition variables.

    This function may sleep, so it must not be called within an
    interrupt handler.  This function may be called with
    interrupts disabled, but interrupts will be turned back on if
    we need to sleep. */
void cond_wait(struct condition *cond, struct lock *lock) {
    struct semaphore_elem waiter;

    ASSERT(cond != NULL);
    ASSERT(lock != NULL);
    ASSERT(!intr_context());
    ASSERT(lock_held_by_current_thread(lock));
  
    sema_init(&waiter.semaphore, 0);
    list_push_back(&cond->waiters, &waiter.elem);
    lock_release(lock);
    sema_down(&waiter.semaphore);
    lock_acquire(lock);
}

/*! If any threads are waiting on COND (protected by LOCK), then
    this function signals one of them to wake up from its wait.
    LOCK must be held before calling this function.

    An interrupt handler cannot acquire a lock, so it does not
    make sense to try to signal a condition variable within an
    interrupt handler. */
void cond_signal(struct condition *cond, struct lock *lock UNUSED) {
    ASSERT(cond != NULL);
    ASSERT(lock != NULL);
    ASSERT(!intr_context ());
    ASSERT(lock_held_by_current_thread (lock));

    if (!list_empty(&cond->waiters)) 
        sema_up(&list_entry(list_pop_front(&cond->waiters),
                            struct semaphore_elem, elem)->semaphore);
}

/*! Wakes up all threads, if any, waiting on COND (protected by
    LOCK).  LOCK must be held before calling this function.

    An interrupt handler cannot acquire a lock, so it does not
    make sense to try to signal a condition variable within an
    interrupt handler. */
void cond_broadcast(struct condition *cond, struct lock *lock) {
    ASSERT(cond != NULL);
    ASSERT(lock != NULL);

    while (!list_empty(&cond->waiters))
        cond_signal(cond, lock);
}

/*! Initializes rw_lock by initializing its lock,
 * read_cond, and write_cond.
 */
void rw_lock_init(struct rw_lock *rw) {
    ASSERT(rw != NULL);

    /* We must initialize the lock, which controls
     * access to rw->state and rw->num_read,
     * initialize the conditions that signal readers
     * and writers to wake up, and set up the rw_lock to
     * be UNLOCKED with no readers.*/
    rw->state = UNLOCKED;      
    lock_init(&rw->lock);
    strlcpy(rw->lock.name, "rw_lock lock", 16);
    cond_init(&rw->read_cond);
    cond_init(&rw->write_cond);
    rw->num_read = 0;
    printf("Initializing rw_locks...\n");
}

/*! Waits until readers are allowed to read. */
void wait_read(struct rw_lock *rw) {
    ASSERT(rw != NULL);

    printf("Waiting to read.\n");
    /* Acquire access to rw->num_read and rw->state. */
    lock_acquire(&rw->lock);

    /* If there is a writer, note that there is a 
     * waiting reader. */
    if (rw->state == WRITE) { 
        rw->state = READER_WAITING;
    }

    /* Wait until we are not writing.
     * We can read if other people are reading, so
     * we only need to check if someone is writing or
     * waiting to write.*/
    while (rw->state != READ && rw->state != UNLOCKED) { 
        cond_wait(&rw->read_cond, &rw->lock);
    }

    /* Release access to rw->num_read and rw->state. */
    lock_release(&rw->lock);
}

/*! Waits until a writer is allowed to write. */
void wait_write(struct rw_lock *rw) { 
    ASSERT(rw != NULL);

    struct thread *t = thread_current();
    printf("Waiting to write %d in thread %s\n", rw->id, t->name);

    /* Acquire access to rw->num_read and rw->state. */
    lock_acquire(&rw->lock);

    /* If there are readers, note that there is a
     * waiting writer. */
    if (rw->state == READ) {
        rw->state = WRITER_WAITING;
    }

    /* We can't write if someone is already writing
     * or reading, so wait until the lock is unlocked. */
    while (rw->state != UNLOCKED) { 
        cond_wait(&rw->write_cond, &rw->lock);
    }

    printf("About to write lock %p\n", &rw->lock);
    /* Release access to rw->num_read and rw->state. */
    lock_release(&rw->lock);
    printf("Released lock %p\n", &rw->lock);
}

/*! Indicate that you are done reading  */
void done_read(struct rw_lock *rw) { 

    /* Acquire access to rw->num_read and rw->state. */
    lock_acquire(&rw->lock);

    /* Only the last reader should signal
     * to the next writer or waiting */
    rw->num_read--;
    if (rw->num_read == 0) {

        /* Prioritize signaling writers to be fair. */
        if (rw->state == WRITER_WAITING) {
            rw->state = WRITE;
            cond_signal(&rw->write_cond, &rw->lock);
        }
        /* Signal readers if there are any. */
        else if (!list_empty(&rw->read_cond.waiters)) { 
            rw->state = READ;
            rw->num_read = list_size(&rw->read_cond.waiters);
            cond_broadcast(&rw->read_cond, &rw->lock);
        }
        /* Otherwise there is no one waiting. */
        else { 
            rw->state = UNLOCKED;
        }
    }

    /* Release access to rw->num_read and rw->state. */
    lock_release(&rw->lock);
}

/*! Indicate that you are done writing. */
void done_write(struct rw_lock *rw) { 
    
    /* Aquire access to rw->num_read and rw->state. */
    lock_acquire(&rw->lock);

    /* Prioritize waking up readers to be fair and
     * avoid writers teaming up to deadlock. */
    if (rw->state == READER_WAITING) { 
        rw->state = READ;
        rw->num_read = list_size(&rw->read_cond.waiters);
        cond_broadcast(&rw->read_cond, &rw->lock);
    }
    /* Signal the next writer if there is one. */
    else if (!list_empty(&rw->write_cond.waiters)) { 
        rw->state = WRITE;
        cond_signal(&rw->write_cond, &rw->lock);
    }
    /* Otherwise there is no one waiting. */
    else { 
        rw->state = UNLOCKED;
    }

    /* Release access to rw->num_read and rw->state. */
    lock_release(&rw->lock);
}
