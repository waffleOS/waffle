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
            int status = *(f->esp + 4);
            exit(status);
            break;
        case SYS_EXEC:
            const char * cmd_line = *(f->esp + 4);
            pid_t pid = exec(cmd_line);
            *f->eax = pid;
           break;
        case SYS_WAIT:
            pid_t pid = *(f->esp + 4);
            int status = wait();
            *f->eax = status;
            break;
        case SYS_CREATE:
            const char * file = *(f->esp + 4);
            unsigned int initial_size = *(f->esp + 8);
            bool success = create(file, initial_size);
            *f->eax = success;
            break;
        case SYS_REMOVE:
            const char * file = *(f->esp + 4);
            bool success = remove(file);
            *f->eax = success;
            break;
        case SYS_FILESIZE:
            int fd = *(f->esp + 4);
            int size = filesize(fd);
            *f->eax = size;
            break;
        case SYS_READ:
            int fd = *(f->esp + 4);
            void * buffer = *(f->esp + 8);
            unsigned int size = *(f->esp + 12);
            int num_bytes = read(fd, buffer, size);
            *f->eax = num_bytes;
            break;
        case SYS_WRITE:
            int fd = *(f->esp + 4);
            void * buffer = *(f->esp + 8);
            unsigned int size = *(f->esp + 12);
            int num_bytes = write(fd, buffer, size);
            *f->eax = num_bytes;
            break;
        case SYS_SEEK:
            int fd = *(f->esp + 4);
            unsigned int position = *(f->esp + 8);
            seek(fd, position);
            break;
        case SYS_TELL:
            int fd = *(f->esp + 4);
            int position = tell(fd);
            *f->eax = position;
            break;
        case SYS_CLOSE:
            int fd = *(f->esp + 4);
            close(fd);
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
