#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "lib/user/syscall.h"

static void syscall_handler(struct intr_frame *);

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler(struct intr_frame *f UNUSED) {
    printf("system call!\n");
    int syscall_num;
    syscall_num = *((int *) f->esp);
    
    // Declarations of arguments
    int status;
    const char * cmd_line;
    pid_t pid;
    const char * file;
    unsigned int initial_size;
    void * buffer;
    unsigned int position;
    int num_bytes;
    int size;
    int fd;
    bool success;

    switch (syscall_num)
    {
        case SYS_HALT:
            halt();
            break;
        case SYS_EXIT:
            status = *((int *) f->esp + 4);
            exit(status);
            break;
        case SYS_EXEC:
            cmd_line = *((const char **) f->esp + 4);
            pid = exec(cmd_line);
            *((pid_t *) f->eax) = pid;
           break;
        case SYS_WAIT:
            pid = *((pid_t *) f->esp + 4);
            status = wait(pid);
            *((int *) f->eax) = status;
            break;
        case SYS_CREATE:
            file = *((const char **) f->esp + 4);
            initial_size = *((int *) f->esp + 8);
            success = create(file, initial_size);
            *((bool *) f->eax) = success;
            break;
        case SYS_REMOVE:
            file = *((const char **) f->esp + 4);
            *((bool *) f->eax) = success;
            break;
        case SYS_FILESIZE:
            fd = *((int *) f->esp + 4);
            size = filesize(fd);
            *((int *) f->eax) = size;
            break;
        case SYS_READ:
            fd = *((int *) f->esp + 4);
            buffer = *(f->esp + 8);
            size = *((unsigned int *) f->esp + 12);
            num_bytes = read(fd, buffer, size);
            *((int *) f->eax) = num_bytes;
            break;
        case SYS_WRITE:
            fd = *((int *) f->esp + 4);
            buffer = *(f->esp + 8);
            size = *((unsigned int *) f->esp + 12);
            num_bytes = write(fd, buffer, size);
            *((int *) f->eax) = num_bytes;
            break;
        case SYS_SEEK:
            fd = *((int *) f->esp + 4);
            position = *((unsigned int *) f->esp + 8);
            seek(fd, position);
            break;
        case SYS_TELL:
            fd = *((int *) f->esp + 4);
            position = tell(fd);
            *((unsigned int *) f->eax) = position;
            break;
        case SYS_CLOSE:
            fd = *((int *) f->esp + 4);
            close(fd);
            break;
    }

    thread_exit();
}

void halt(void)
{
    shutdown_power_off();    
}

void exit(int status)
{
    thread_exit();
}

pid_t exec(const char * cmd_line)
{

}

int wait(pid_t pid)
{

}

bool create(const char * file, unsigned int initial_size)
{

}

bool remove(const char * file)
{

}

int open(const char * file)
{

}

int filesize(int fd)
{

}

int read(int fd, void * buffer, unsigned int size)
{

}

int write(int fd, const void * buffer, unsigned int size)
{

}

void seek(int fd, unsigned int position)
{

}

unsigned int tell(int fd)
{

}

void close (int fd)
{

}
