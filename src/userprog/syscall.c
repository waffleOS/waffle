#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "lib/user/syscall.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

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
bool validate_pointer(void *ptr);

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler(struct intr_frame *f UNUSED) {
    int syscall_num;
    syscall_num = *((int *) f->esp);
    printf("system call %d\n", syscall_num);
    
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
            status = *((int *) (f->esp + 4));
            do_exit(status);
            break;
        case SYS_EXEC:
            cmd_line = *((const char **) (f->esp + 4));
            pid = do_exec(cmd_line);
            *((pid_t *) f->eax) = pid;
           break;
        case SYS_WAIT:
            pid = *((pid_t *) (f->esp + 4));
            status = do_wait(pid);
            *((int *) f->eax) = status;
            break;
        case SYS_CREATE:
            file = *((const char **) (f->esp + 4));
            initial_size = *((int *) (f->esp + 8));
            success = do_create(file, initial_size);
            *((bool *) f->eax) = success;
            break;
        case SYS_REMOVE:
            file = *((const char **) (f->esp + 4));
            success = do_remove(file);
            *((bool *) f->eax) = success;
            break;
        case SYS_OPEN:
            file = *((const char **) (f->esp + 4));
            fd = do_open(file);
            printf("Opened file with fd %d\n", fd);
            *((int *) f->eax) = fd;
            break;
        case SYS_FILESIZE:
            fd = *((int *) (f->esp + 4));
            size = do_filesize(fd);
            *((int *) f->eax) = size;
            break;
        case SYS_READ:
            fd = *((int *) (f->esp + 4));
            buffer = *((void **) (f->esp + 8));
            size = *((unsigned int *) (f->esp + 12));
            num_bytes = do_read(fd, buffer, size);
            *((int *) f->eax) = num_bytes;
            break;
        case SYS_WRITE:
            fd = *((int *) (f->esp + 4));
            buffer = *((void **) (f->esp + 8));
            size = *((unsigned int *) (f->esp + 12));
            num_bytes = do_write(fd, buffer, size);
            *((int *) f->eax) = num_bytes;
            break;
        case SYS_SEEK:
            fd = *((int *) (f->esp + 4));
            position = *((unsigned int *) (f->esp + 8));
            do_seek(fd, position);
            break;
        case SYS_TELL:
            fd = *((int *) (f->esp + 4));
            position = do_tell(fd);
            *((unsigned int *) f->eax) = position;
            break;
        case SYS_CLOSE:
            fd = *((int *) (f->esp + 4));
            do_close(fd);
            break;
    }

}

void do_halt(void)
{
    shutdown_power_off();
}

void do_exit(int status)
{
    struct thread *cur = thread_current();
    printf("%s:exit(%d)\n", cur->name, status);
    thread_exit();
}

pid_t do_exec(const char * cmd_line)
{
    /* For ease, let's say process ids and thread ids line up in a one-to-one
    mapping */
    pid_t child = process_execute(cmd_line);

    /* TODO: Implement synchronization */

    if(child == TID_ERROR) {
        return -1;
    }
    return child;
}

int do_wait(pid_t pid)
{
    process_wait(pid);
}

bool do_create(const char * file, unsigned int initial_size)
{
    printf("Creating file %s with size %d\n", file, initial_size);
    return filesys_create(file, initial_size);
}

bool do_remove(const char * file)
{
    printf("Removing file %s\n", file);
    return filesys_remove(file);
}

int do_open(const char * file)
{
    printf("Opening file %s\n", file);    
    struct file * f= filesys_open(file); 
    struct thread * t = thread_current();
    int fd = next_fd(t);
    t->files[fd - 2] = f;
    return fd;
}

int do_filesize(int fd)
{
    printf("Getting filesize of file with fd %d\n", fd);
    struct thread * t = thread_current();
    return file_length(t->files[fd - 2]); 
}

int do_read(int fd, void * buffer, unsigned int size)
{
    printf("Reading file with fd %d\n", fd);
    if (fd == 0)
    {
        int i;
        for (i = 0; i < size; i++)
        {
            *((char *) buffer + i) = input_getc();
        }
        return size;
    }

    struct thread * t = thread_current();
    return file_read(t->files[fd - 2], buffer, size); 
}

int do_write(int fd, const void * buffer, unsigned int size)
{
    printf("In do_write... fd is %d, buffer is %s, size is %d\n", fd, (char *) buffer, size);
    if (fd == 1)
    {
        putbuf((char *) buffer, size);
        return size;
    }

    struct thread * t = thread_current();
    return file_write(t->files[fd - 2], buffer, size);
}

void do_seek(int fd, unsigned int position)
{
    printf("Seeking file with fd %d to position %d\n", fd, position);
    struct thread * t = thread_current();
    file_seek(t->files[fd - 2], position);
}

unsigned int do_tell(int fd)
{
    struct thread * t = thread_current();
    return file_tell(t->files[fd - 2]);
}

void do_close (int fd)
{
    printf("Closing file with fd %d\n", fd);
    struct thread * t = thread_current();
    file_close(t->files[fd - 2]);
}

bool validate_pointer(void *ptr) {
    /* Check if pointer is in correct space. */
    if (ptr == NULL || !is_user_vaddr(ptr)) {
        process_exit();
        return false;
    }
    
    struct thread *cur = thread_current();
    uint32_t *pd = cur->pagedir;

    /* Check if in page directory of the current thread. */
    if (pagedir_get_page(pd, ptr) == NULL) {
        process_exit();
        return false;
    }

    return true;
}
