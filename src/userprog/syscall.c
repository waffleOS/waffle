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
void * sanitize_buffer(void ** buffer);

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    sema_init(&file_sem, 1);
}

static void syscall_handler(struct intr_frame *f UNUSED) {
    int syscall_num;

    // Check for Stack pointer corrupted.
    if(!validate_pointer((void *)(f->esp))) {
        do_exit(-1);
        return;
    }

    // Check stack pointer isn't too far up
    if(!validate_pointer((void *)(f->esp - 4))) {
        do_exit(-1);
        return;
    }

    syscall_num = *((int *) f->esp);
    /*printf("system call %d\n", syscall_num);*/
    
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
            f->eax = pid;
           break;
        case SYS_WAIT:
            pid = *((pid_t *) (f->esp + 4));
            status = do_wait(pid);
            f->eax = status;
            break;
        case SYS_CREATE:
            file = *((const char **) (f->esp + 4));
            initial_size = *((int *) (f->esp + 8));
            if (validate_pointer(file)) {
                success = do_create(file, initial_size);
                f->eax = success;
            }
            else {
                do_exit(-1);
            }
            break;
        case SYS_REMOVE:
            file = *((const char **) (f->esp + 4));
            success = do_remove(file);
            f->eax = success;
            break;
        case SYS_OPEN:
            file = *((const char **) (f->esp + 4));
            if (validate_pointer(file)) {
                fd = do_open(file);
                f->eax = fd;
            }
            else {
                do_exit(-1);
            }
            break;
        case SYS_FILESIZE:
            fd = *((int *) (f->esp + 4));
            size = do_filesize(fd);
            f->eax = size;
            break;
        case SYS_READ:
            fd = *((int *) (f->esp + 4));
            buffer = sanitize_buffer((void **) (f->esp + 8));
            if (validate_pointer(buffer)) {
                size = *((unsigned int *) (f->esp + 12));
                num_bytes = do_read(fd, buffer, size);
                f->eax = num_bytes;
            }
            else {
                do_exit(-1);
            }
            break;
        case SYS_WRITE:
            fd = *((int *) (f->esp + 4));
            buffer = *((void **) (f->esp + 8));
            if(validate_pointer(buffer)) {
                size = *((unsigned int *) (f->esp + 12));
                num_bytes = do_write(fd, buffer, size);
                f->eax = num_bytes;
            }
            else {
                do_exit(-1);
            }
            break;
        case SYS_SEEK:
            fd = *((int *) (f->esp + 4));
            position = *((unsigned int *) (f->esp + 8));
            do_seek(fd, position);
            break;
        case SYS_TELL:
            fd = *((int *) (f->esp + 4));
            position = do_tell(fd);
            f->eax = position;
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
    
    char *saveptr;
    char *name;
    name = strtok_r(cur->name, " ", &saveptr);
    printf("%s: exit(%d)\n", name, status);
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
    return process_wait(pid);
}

bool do_create(const char * file, unsigned int initial_size)
{
    if (file == NULL)
    {
        do_exit(-1);
    }
    sema_down(&file_sem);
    /*printf("Creating file %s with size %d\n", file, initial_size);*/
    bool success = filesys_create(file, initial_size);
    sema_up(&file_sem);
    return success;
}

bool do_remove(const char * file)
{
    sema_down(&file_sem);
    /*printf("Removing file %s\n", file);*/
    bool success = filesys_remove(file);
    sema_up(&file_sem);
    return success;
}

int do_open(const char * file)
{
    if (file == NULL)
    {
        do_exit(-1);
    }
    sema_down(&file_sem);
    /*printf("Opening file %s\n", file);*/
    struct file * f = filesys_open(file);
    sema_up(&file_sem);
    if (f == NULL)
    {
        return -1;
    }
    struct thread * t = thread_current();
    int fd = next_fd(t);
    t->files[fd - 2] = f;
    return fd;
}

int do_filesize(int fd)
{
    /*printf("Getting filesize of file with fd %d\n", fd);*/
    struct thread * t = thread_current();
    sema_down(&file_sem);
    int length = file_length(t->files[fd - 2]);
    sema_up(&file_sem);
    return length;
}

int do_read(int fd, void * buffer, unsigned int size)
{
    /*printf("Reading file with fd %d\n", fd);*/
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
    if (is_valid_fd(t, fd)) {
        sema_down(&file_sem);
        int length = file_read(t->files[fd - 2], buffer, size); 
        sema_up(&file_sem);
        return length;
    }

    return -1;
}

int do_write(int fd, const void * buffer, unsigned int size)
{
    if (fd == 0)
    {
        return 0;
    }

    if (fd == 1)
    {
        putbuf((char *) buffer, size);
        return size;
    }

    struct thread * t = thread_current();
    if (is_valid_fd(t, fd))
    {
        sema_down(&file_sem);
        int length = file_write(t->files[fd - 2], buffer, size);
        sema_up(&file_sem);
        return length;
    }

    return 0;
}

void do_seek(int fd, unsigned int position)
{
    /*printf("Seeking file with fd %d to position %d\n", fd, position);*/
    struct thread * t = thread_current();
    sema_down(&file_sem);
    file_seek(t->files[fd - 2], position);
    sema_up(&file_sem);
}

unsigned int do_tell(int fd)
{
    struct thread * t = thread_current();
    sema_down(&file_sem);
    int position = file_tell(t->files[fd - 2]);
    sema_up(&file_sem);
}

void do_close(int fd)
{
    if (fd < 2)
    {
        /*close(fd);*/
    }
    else
    {
        /*printf("Closing file with fd %d\n", fd);*/
        struct thread * t = thread_current();
        if (is_valid_fd(t, fd)) {
            sema_down(&file_sem);
            file_close(t->files[fd - 2]);
            sema_up(&file_sem);
            t->files[fd - 2] = NULL;
        }
    }
}

bool validate_pointer(void *ptr) {
    /* Check if pointer is in correct space. */
    if (ptr == NULL || !is_user_vaddr(ptr)) {
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

void * sanitize_buffer(void ** buffer)
{
    if (!validate_pointer(buffer))
    {
        do_exit(-1);
    }

    return *buffer;
}
