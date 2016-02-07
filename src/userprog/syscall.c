#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"

static void syscall_handler(struct intr_frame *);

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler(struct intr_frame *f UNUSED) {
    printf("system call!\n");
    int syscall_num = *f->esp;
    switch (syscall_num)
    {
        case SYS_HALT:
            halt();
            break;
        case SYS_EXIT:
            exit(status);
            break;
        case SYS_EXEC:
            exec();
            break;
        case SYS_WAIT:
            wait();
            break;
        case SYS_CREATE:
            create();
            break;
        case SYS_REMOVE:
            remove();
            break;
        case SYS_FILESIZE:
            filesize();
            break;
        case SYS_READ:
            read();
            break;
        case SYS_WRITE:
            write();
            break;
        case SYS_SEEK:
            seek();
            break;
        case SYS_TELL:
            tell();
            break;
        case SYS_CLOSE:
            close();
            break;
    }

    thread_exit();
}

void halt(void)

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
