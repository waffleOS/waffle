/*! \file synch.h
 *
 * Data structures and function declarations for thread synchronization
 * primitives.
 */

#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>

/*! A counting semaphore. */
struct semaphore {
    unsigned value;             /*!< Current value. */
    struct list waiters;        /*!< List of waiting threads. */
};

void sema_init(struct semaphore *, unsigned value);
void sema_down(struct semaphore *);
bool sema_try_down(struct semaphore *);
void sema_up(struct semaphore *);
void sema_self_test(void);

/*! Lock. */
struct lock {
    struct thread *holder;      /*!< Thread holding lock (for debugging). */
    struct semaphore semaphore; /*!< Binary semaphore controlling access. */
};

void lock_init(struct lock *);
void lock_acquire(struct lock *);
bool lock_try_acquire(struct lock *);
void lock_release(struct lock *);
bool lock_held_by_current_thread(const struct lock *);

/*! Condition variable. */
struct condition {
    struct list waiters;        /*!< List of waiting threads. */
};

void cond_init(struct condition *);
void cond_wait(struct condition *, struct lock *);
void cond_signal(struct condition *, struct lock *);
void cond_broadcast(struct condition *, struct lock *);

/*! Possible states of the read-write lock. */
enum rw_state { 
    UNLOCKED, 
    READ,
    WRITE,
    WRITER_WAITING,
    READER_WAITING,
};

/*! Read write lock. */
struct rw_lock { 
    enum rw_state state;         /*!< State of the read-write lock. */
    struct lock lock;            /*!< Lock to share reading and writing. */
    struct condition read_cond;  /*!< Condition to wake up waiting readers. */
    struct condition write_cond; /*!< Condition to wake up a writer. */
    int num_read;                /*!< Number of "active" readers. */
};


void rw_lock_init(struct rw_lock *); 
void wait_read(struct rw_lock *);
void wait_write(struct rw_lock *);
void done_read(struct rw_lock *);
void done_write(struct rw_lock *);

/*! Optimization barrier.

   The compiler will not reorder operations across an
   optimization barrier.  See "Optimization Barriers" in the
   reference guide for more information.*/
#define barrier() asm volatile ("" : : : "memory")

#endif /* threads/synch.h */

