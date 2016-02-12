#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "lib/user/syscall.h"

/* Module specific function prototypes. */
static void syscall_handler(struct intr_frame *);
void do_halt(void);
void do_exit(int status);
pid_t do_exec(const char * cmd_line);
int do_wait(pid_t pid);
bool do_create(const char * file, unsigned int initial_size);
bool do_remove(const char * file);
int do_open(const char * file);
int do_filesize(int fd);
int do_read(int fd, void * buffer, unsigned int size);
int do_write(int fd, const void * buffer, unsigned int size);
void do_seek(int fd, unsigned int position);
unsigned int do_tell(int fd);
void do_close(int fd);

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
            do_halt();
            break;
        case SYS_EXIT:
            status = *((int *) f->esp + 4);
            do_exit(status);
            break;
        case SYS_EXEC:
            cmd_line = *((const char **) f->esp + 4);
            pid = do_exec(cmd_line);
            *((pid_t *) f->eax) = pid;
           break;
        case SYS_WAIT:
            pid = *((pid_t *) f->esp + 4);
            status = do_wait(pid);
            *((int *) f->eax) = status;
            break;
        case SYS_CREATE:
            file = *((const char **) f->esp + 4);
            initial_size = *((int *) f->esp + 8);
            success = do_create(file, initial_size);
            *((bool *) f->eax) = success;
            break;
        case SYS_REMOVE:
            file = *((const char **) f->esp + 4);
            success = do_remove(file);
            *((bool *) f->eax) = success;
            break;
        case SYS_FILESIZE:
            fd = *((int *) f->esp + 4);
            size = do_filesize(fd);
            *((int *) f->eax) = size;
            break;
        case SYS_READ:
            fd = *((int *) f->esp + 4);
            buffer = *((void **) f->esp + 8);
            size = *((unsigned int *) f->esp + 12);
            num_bytes = do_read(fd, buffer, size);
            *((int *) f->eax) = num_bytes;
            break;
        case SYS_WRITE:
            fd = *((int *) f->esp + 4);
            buffer = *((void **) f->esp + 8);
            size = *((unsigned int *) f->esp + 12);
            num_bytes = do_write(fd, buffer, size);
            *((int *) f->eax) = num_bytes;
            break;
        case SYS_SEEK:
            fd = *((int *) f->esp + 4);
            position = *((unsigned int *) f->esp + 8);
            do_seek(fd, position);
            break;
        case SYS_TELL:
            fd = *((int *) f->esp + 4);
            position = do_tell(fd);
            *((unsigned int *) f->eax) = position;
            break;
        case SYS_CLOSE:
            fd = *((int *) f->esp + 4);
            do_close(fd);
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

int do_wait(pid_t pid)
{

}

bool do_create(const char * file, unsigned int initial_size)
{

}

bool do_remove(const char * file)
{

}

int do_open(const char * file)
{

}

int do_filesize(int fd)
{

}

int do_read(int fd, void * buffer, unsigned int size)
{

}

int do_write(int fd, const void * buffer, unsigned int size)
{

}

void do_seek(int fd, unsigned int position)
{

}

unsigned int do_tell(int fd)
{

}

void do_close (int fd)
{

}

bool validatePointer(void *ptr) {
    if(ptr == NULL) return false;
    if(!is_user_vaddr(ptr)) return false;
    if(pagedir_get_page(pd, ptr) == NULL) return false;
    return true;
}
