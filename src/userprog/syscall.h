#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/synch.h"

struct lock file_lock;
struct semaphore exec_sem;
struct lock exec_lock;
struct condition exec_cond;
bool load_success;

void syscall_init(void);
void do_exit(int status);
#endif /* userprog/syscall.h */

