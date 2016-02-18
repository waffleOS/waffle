#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/synch.h"

struct semaphore file_sem;
struct semaphore exec_sem;

void syscall_init(void);
void do_exit(int status);
#endif /* userprog/syscall.h */

