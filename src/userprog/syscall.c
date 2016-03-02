#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "devices/shutdown.h"
#include "lib/user/syscall.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "vm/page.h"
#include "vm/structs.h"
#include "threads/malloc.h"

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
mapid_t do_mmap(int fd, void *addr);
void do_munmap(mapid_t mapping);

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    sema_init(&file_sem, 1);
    sema_init(&exec_sem, 1);
    lock_init(&exec_lock);
    cond_init(&exec_cond);
}

static void syscall_handler(struct intr_frame *f UNUSED) {
    int syscall_num;

    /*printf("Starting a system call\n");*/
    // Check for Stack pointer corrupted.
    if(!validate_pointer((void *)(f->esp))) {
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
    uint8_t * addr;
    mapid_t map;

    switch (syscall_num)
    {
        case SYS_HALT:
            do_halt();
            break;
        case SYS_EXIT:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            status = *((int *) (f->esp + 4));
            do_exit(status);
            break;
        case SYS_EXEC:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            cmd_line = *((const char **) (f->esp + 4));
            if (validate_pointer(cmd_line)) {
                pid = do_exec(cmd_line);
                /*if (pid == TID_ERROR) {
                    return;
                }*/
                f->eax = pid;
            }
            else {
                f->eax = -1;
            }
           break;
        case SYS_WAIT:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            pid = *((pid_t *) (f->esp + 4));
            status = do_wait(pid);
            f->eax = status;
            break;
        case SYS_CREATE:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 8))) {
                do_exit(-1);
                return;
            }
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
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            file = *((const char **) (f->esp + 4));
            success = do_remove(file);
            f->eax = success;
            break;
        case SYS_OPEN:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            /*printf("Printing inside open\n");*/
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
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            fd = *((int *) (f->esp + 4));
            size = do_filesize(fd);
            f->eax = size;
            break;
        case SYS_READ:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 12))) {
                do_exit(-1);
                return;
            }
            /*printf("Printing inside read\n");*/
            fd = *((int *) (f->esp + 4));
            buffer = sanitize_buffer((void **) (f->esp + 8));
            /*printf("Done sanitizing inside read\n");*/
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
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 12))) {
                do_exit(-1);
                return;
            }
            /*printf("Printing inside write\n");*/
            fd = *((int *) (f->esp + 4));
            buffer = sanitize_buffer((void **) (f->esp + 8));
            /*printf("Done sanitizing inside write\n");*/
            if(validate_pointer(buffer)) {
                size = *((unsigned int *) (f->esp + 12));
                /*printf("Got here\n");*/
                /*printf("The buffer looks like %s\n", (char *) buffer);*/
                num_bytes = do_write(fd, buffer, size);
                f->eax = num_bytes;
                /*printf("Number of bytes: %d\n", num_bytes);*/
            }
            else {
                do_exit(-1);
            }
            /*printf("Got here 2\n");*/
            break;
        case SYS_SEEK:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 8))) {
                do_exit(-1);
                return;
            }
            fd = *((int *) (f->esp + 4));
            position = *((unsigned int *) (f->esp + 8));
            do_seek(fd, position);
            break;
        case SYS_TELL:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            fd = *((int *) (f->esp + 4));
            position = do_tell(fd);
            f->eax = position;
            break;
        case SYS_CLOSE:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            fd = *((int *) (f->esp + 4));
            do_close(fd);
            break;
        case SYS_MMAP:
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            fd = *((int *) (f->esp + 4));
            addr = *((uint8_t **) (f->esp + 8));
            map = do_mmap(fd, addr);
            f->eax = map;
            break;
        case SYS_MUNMAP:
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            map = *((int *) (f->esp + 4));
            do_munmap(fd);
            break;

    }

}
/* Halts the systems by shutting down */
void do_halt(void)
{
    shutdown_power_off();
}

/* Exits the current thread and prints a status */
void do_exit(int status)
{
    struct thread *cur = thread_current();
    
    cur->exit_status = status;
    char *saveptr;
    char *name;
    name = strtok_r(cur->name, " ", &saveptr);
    printf("%s: exit(%d)\n", name, status);
    thread_exit();
}

/* Executes a command and spawns a new thread */
pid_t do_exec(const char * cmd_line)
{
    struct thread *cur = thread_current();
    
    /* For ease, let's say process ids and thread ids line up in a one-to-one
    mapping */

    // Make sure the command is not NULL
    if (cmd_line == NULL)
    {
        do_exit(-1);
    }

    /* Load and execute child. */
    sema_down(&exec_sem);
    pid_t child = process_execute(cmd_line);
    sema_up(&exec_sem);

    sema_down(&cur->load_sem);

    
    /* Check if a thread was allocated. */
    if (child == TID_ERROR) {
        do_exit(-1);
        return TID_ERROR;
    }

    /* Check if the child loaded successfully. */
    if (cur->load_success) { 
        return child;
    }
    else { 
        return TID_ERROR;
    }
}

/* Waits on a process */
int do_wait(pid_t pid)
{
    return process_wait(pid);
}

/* Creates a new file */
bool do_create(const char * file, unsigned int initial_size)
{
    // If the file is NULL, exit the thread
    if (file == NULL)
    {
        do_exit(-1);
    }

    // Entering critical code
    sema_down(&file_sem);
    bool success = filesys_create(file, initial_size);
    sema_up(&file_sem);
    // Left critical code
    
    return success;
}

/* Removes a new file */
bool do_remove(const char * file)
{
    sema_down(&file_sem);
    bool success = filesys_remove(file);
    sema_up(&file_sem);
    return success;
}

/* Opens a new file */
int do_open(const char * file)
{
    if (file == NULL)
    {
        do_exit(-1);
    }
    sema_down(&file_sem);
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

/* Returns the size of a file */
int do_filesize(int fd)
{
    struct thread * t = thread_current();
    sema_down(&file_sem);
    if (!is_valid_fd(t, fd))
    {
        return 0;
    }
    int length = file_length(t->files[fd - 2]);
    sema_up(&file_sem);
    return length;
}

/* Reads from a file */
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

    struct thread * t = thread_current();
    if (is_valid_fd(t, fd)) {
        sema_down(&file_sem);
        int length = file_read(t->files[fd - 2], buffer, size); 
        sema_up(&file_sem);
        return length;
    }

    return -1;
}

/* Writes to a file */
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

/* Seeks in a file */
void do_seek(int fd, unsigned int position)
{
    struct thread * t = thread_current();
    sema_down(&file_sem);
    file_seek(t->files[fd - 2], position);
    sema_up(&file_sem);
}

/* Returns the position of a file */
unsigned int do_tell(int fd)
{
    struct thread * t = thread_current();
    sema_down(&file_sem);
    int position = file_tell(t->files[fd - 2]);
    sema_up(&file_sem);
    return position;
}

/* Closes a file */
void do_close(int fd)
{
    if (fd >= 2)
    {
        struct thread * t = thread_current();
        if (is_valid_fd(t, fd)) {
            sema_down(&file_sem);
            file_close(t->files[fd - 2]);
            sema_up(&file_sem);
            t->files[fd - 2] = NULL;
        }
    }
}

mapid_t do_mmap(int fd, void *addr)
{
    if (fd == 0 || fd == 1)
    {
        return -1;
    }

    if (addr == 0 || pg_ofs(addr) != 0)
    {
        return -1;
    }

    struct thread * t = thread_current();

    if (!is_valid_fd(t, fd))
    {
        return -1;
    }

    struct file * f = file_reopen(t->files[fd - 2]);
    int read_bytes = do_filesize(fd);
    if (read_bytes == 0)
    {
        return -1;
    }

    int ofs = 0;
    bool writable = true;
    unsigned int num_pages = 0;
    uint8_t * upage = addr;

    do_seek(fd, 0);
    while (read_bytes > 0) {
        /* Calculate how to fill this page.
           We will read PAGE_READ_BYTES bytes from FILE
           and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        struct page_info * p_info = install_page_info(addr, f, ofs, page_read_bytes, page_zero_bytes, writable, MMAP_FILE);
        if (p_info == NULL)
        {
            while (num_pages > 0)
            {
                page_info_delete(&t->sup_page_table, upage);
                upage += PGSIZE;
            }
            return -1;
        }

        /* Advance. */
        read_bytes -= page_read_bytes;
        addr += PGSIZE;
        ofs += page_read_bytes;
        num_pages++;
    }

    struct mapping * m = (struct mapping *) malloc(sizeof(struct mapping));
    m->upage = upage;
    m->file = f;
    m->num_pages = num_pages;
    int map = next_map(t);     
    t->mappings[map] = m; 
}

void do_munmap(mapid_t mapping)
{
    struct thread * t = thread_current();
    if (!is_valid_map(t, mapping))
    {
       return; 
    }

    struct mapping * m = t->mappings[mapping];
    uint8_t * upage = m->upage;
    int filesize = file_length(m->file);
    int ofs = 0;
    int i;
    for (i = 0; i < m->num_pages; i++)
    {
        size_t page_write_bytes = filesize - ofs < PGSIZE ? filesize - ofs : PGSIZE;
        if (pagedir_is_dirty(t->pagedir, upage))
        {
            file_write(m->file, upage, page_write_bytes);
        }

        page_info_delete(&t->sup_page_table, upage);
        pagedir_clear_page(t->pagedir, upage);

        upage += PGSIZE;
        ofs += PGSIZE;
    }

    t->mappings[mapping] = NULL;
    file_close(m->file);
}

/* Validates pointers */
bool validate_pointer(void *ptr) {
    /* Check if pointer is in correct space. */
    if (ptr == NULL || !is_user_vaddr(ptr)) {
        return false;
    }
    
    struct thread *cur = thread_current();
    uint32_t *pd = cur->pagedir;
    /* Check if in page directory of the current thread. */
    if (pagedir_get_page(pd, ptr) == NULL) {
        return false;
    }

    return true;
}

/* Validates pointers and dereferences */
void * sanitize_buffer(void ** buffer)
{
    if (!validate_pointer(buffer))
    {
        do_exit(-1);
    }

    return *buffer;
}
