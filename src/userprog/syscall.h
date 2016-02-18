#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/synch.h"

struct semaphore file_sem;

void syscall_init(void);
#endif /* userprog/syscall.h */

