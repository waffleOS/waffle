#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define NUM_FILES
void syscall_init(void);
int last_fd_used;
struct file * files[NUM_FILES];
#endif /* userprog/syscall.h */

