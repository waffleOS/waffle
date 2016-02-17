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
    last_fd_used = 2;
}

static void syscall_handler(struct intr_frame *f UNUSED) {
    int syscall_num;
    printf("ESP: 0x%8x\n", f->esp);
    syscall_num = *((int *) f->esp);
    printf("system call %d. SYS_EXIT: %d\n", syscall_num, SYS_EXIT);
    
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

    thread_exit();
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
    filesys_create(file, initial_size);
}

bool do_remove(const char * file)
{
    filesys_remove(file);
}

int do_open(const char * file)
{
   struct file * f= filesys_open(file); 
   files[++last_fd_used] = f;
   return last_fd_used;
}

int do_filesize(int fd)
{
    return file_length(files[fd]); 
}

int do_read(int fd, void * buffer, unsigned int size)
{
    if (fd == 0)
    {
        int i;
        for (i = 0; i < size; i++)
        {
            *((char *) buffer + i) = input_getc();
        }
        return size;
    }

    return file_read(files[fd], buffer, size); 
}

int do_write(int fd, const void * buffer, unsigned int size)
{
    if (fd == 1)
    {
        putbuf((char *) buffer, size);
        return size;
    }

    return file_write(files[fd], buffer, size);
}

void do_seek(int fd, unsigned int position)
{
    file_seek(files[fd], position);
}

unsigned int do_tell(int fd)
{
    return file_tell(files[fd]);
}

void do_close (int fd)
{
    file_close(files[fd]);
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
