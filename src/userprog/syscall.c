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
#include "filesys/inode.h"
#include "filesys/cache.h"

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
bool do_chdir(const char *dir);
bool do_mkdir(const char *dir);
bool do_readdir(int fd, char *name);
bool do_isdir(int fd);
int do_inumber(int fd);
bool validate_pointer(void *ptr);
void * sanitize_buffer(void ** buffer);

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
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
    const char * dir;
    char * name;
    unsigned int initial_size;
    void * buffer;
    unsigned int position;
    int num_bytes;
    int size;
    int fd;
    bool success;
    int inumber;


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
            // printf("Printing inside open\n");
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
                if(num_bytes == -1) {
                    do_exit(-1);
                }
                f->eax = num_bytes;
                // printf("YODELYEHEHOO\n");
                // f->eax = -1;
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

        case SYS_CHDIR:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            dir = *((const char **) (f->esp + 4));
            if (validate_pointer(dir)) {
                success = do_chdir(dir);
                f->eax = success;
            } else {
                do_exit(-1);
            }

            break;
        case SYS_MKDIR:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            dir = *((const char **) (f->esp + 4));
            if (validate_pointer(dir)) {

                success = do_mkdir(dir);
                f->eax = success;
            } else {
                do_exit(-1);
            }

            break;
        case SYS_READDIR:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 8))) {
                do_exit(-1);
                return;
            }
            fd = *((int *) (f->esp + 4));
            name = *((char **) (f->esp + 8));
            // printf("SYS_READDIR fd = %d, name = %s\n", fd, name);
            if (validate_pointer(name)) {
                success = do_readdir(fd, name);
                f->eax = success;
            } else {
                do_exit(-1);
            }

            break;
        case SYS_ISDIR:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            fd = *((int *) (f->esp + 4));
            success = do_isdir(fd);
            f->eax = success;

            break;
        case SYS_INUMBER:
            // Check stack pointer isn't too far up
            if(!validate_pointer((void *)(f->esp + 4))) {
                do_exit(-1);
                return;
            }
            fd = *((int *) (f->esp + 4));
            inumber = do_inumber(fd);
            f->eax = inumber;
            
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

    bool success = filesys_create(file, initial_size);
    // printf("successs for do_create = %d\n", success);
    return success;
}

/* Removes a new file */
bool do_remove(const char * file)
{
    bool success = filesys_remove(file);
    return success;
}

/* Opens a new file */
int do_open(const char * file)
{
    if (file == NULL)
    {
        do_exit(-1);
    }
    struct file * f = filesys_open(file);
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
    int length = file_length(t->files[fd - 2]);
    return length;
}

/* Reads from a file */
int do_read(int fd, void * buffer, unsigned int size)
{
    if (fd == 0)
    {
        unsigned int i;
        for (i = 0; i < size; i++)
        {
            *((char *) buffer + i) = input_getc();
        }
        return size;
    }

    struct thread * t = thread_current();
    if (is_valid_fd(t, fd)) {
        int length = file_read(t->files[fd - 2], buffer, size); 
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
        if(inode_is_dir(t, fd)) {
            return -1;
        }
/*    printf("HEY IM WRITING this buffer: %s\n", buffer);*/
        int length = file_write(t->files[fd - 2], buffer, size);
/*        printf("length = %d\n", length);*/
        return length;
    }

    return 0;
}

/* Seeks in a file */
void do_seek(int fd, unsigned int position)
{
    struct thread * t = thread_current();
    file_seek(t->files[fd - 2], position);
}

/* Returns the position of a file */
unsigned int do_tell(int fd)
{
    struct thread * t = thread_current();
    int position = file_tell(t->files[fd - 2]);
    return position;
}

/* Closes a file */
void do_close(int fd)
{
    if (fd >= 2)
    {
        struct thread * t = thread_current();
        if (is_valid_fd(t, fd)) {
            file_close(t->files[fd - 2]);
            t->files[fd - 2] = NULL;
        }
    }
}


bool do_chdir(const char *dir) {
    return dir_chdir(dir);
}

bool do_mkdir(const char *dir) {
    return dir_mkdir(dir, true, 512);
}

bool do_readdir(int fd, char *name) {
/*    struct list_elem *e;
    for (e = list_begin(&all_list); e != list_end(&all_list);
         e = list_next(e)) {
        struct thread *t = list_entry(e, struct thread, allelem);
        if (fd >= 2 && fd < NUM_FILES && t->files[fd - 2] != NULL)
        {
            return file_get_inode(t->files[fd - 2])->sector;
        }
    }*/
        // printf("HELLO DO AREADDIR fd = %d, name = %s\n", fd, name);
    struct thread * t = thread_current();
    if (is_valid_fd(t, fd))
    {
        // printf("IT IS VALID fd\n");
        return inode_readdir(t, fd, name);
    }
    return false;
}

bool do_isdir(int fd) {
/*    struct thread * t = thread_current();
    int length = file_length(t->files[fd - 2]);
    return length;
*/
    /* Loop through all the threads out there looking for the fd. */
    // struct list_elem *e;
    // struct list all_list = get_all_list();
    // for (e = list_begin(&all_list); e != list_end(&all_list);
    //      e = list_next(e)) {
    //     struct thread *t = list_entry(e, struct thread, allelem);
    //     if (fd >= 2 && fd < NUM_FILES && t->files[fd - 2] != NULL)
    //     {
    //         struct inode *inode = file_get_inode(t->files[fd - 2]);
    //         printf("%d", inode);
    //         // cache_sector * sector = cache_write_sector(inode->sector);
    //         /*struct inode_disk * disk_inode = (struct inode_disk *) sector->data;
    //         return disk_inode->isDirectory;*/

    //         /*cache_sector *sector = cache_read_sector(inode->sector);
    //         struct inode_disk *data = (struct inode_disk *) sector->data;
    //         bool result = data->isDirectory;
    //         done_read(&sector->rw);
    //         return result;*/
    //     }
    // }
    struct thread * t = thread_current();
    if (is_valid_fd(t, fd))
    {
        return inode_is_dir(t, fd);
    }
    return false;
}

int do_inumber(int fd) {
    // struct list_elem *e;
    // for (e = list_begin(&all_list); e != list_end(&all_list);
    //      e = list_next(e)) {
    //     struct thread *t = list_entry(e, struct thread, allelem);
    //     if (fd >= 2 && fd < NUM_FILES && t->files[fd - 2] != NULL)
    //     {
    //         return file_get_inode(t->files[fd - 2])->sector;
    //     }
    // }
    // return -1;
    struct thread * t = thread_current();
    if (is_valid_fd(t, fd))
    {
        // return inode_get_inumber_from_fd(fd);

        return inode_get_inumber_from_fd(t, fd);
    }
    return -1;
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
