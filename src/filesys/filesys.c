#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/cache.h"

/*! Partition that contains the file system. */
struct block *fs_device;

static void do_format(void);

/*! Initializes the file system module.
    If FORMAT is true, reformats the file system. */
void filesys_init(bool format) {
    cache_init();

    fs_device = block_get_role(BLOCK_FILESYS);
    if (fs_device == NULL)
        PANIC("No file system device found, can't initialize file system.");

    inode_init();
    free_map_init();

    if (format) 
        do_format();

    free_map_open();
}

/*! Shuts down the file system module, writing any unwritten data to disk. */
void filesys_done(void) {
    free_map_close();
}

/*! Creates a file named NAME with the given INITIAL_SIZE.  Returns true if
    successful, false otherwise.  Fails if a file named NAME already exists,
    or if internal memory allocation fails. */
bool filesys_create(const char *name, off_t initial_size) {
    // block_sector_t inode_sector = 0;
    /*struct dir *dir = dir_open_root();*/

    // struct thread * t = thread_current();
    // struct dir * dir = t->curdir;

    // if (dir == NULL)
    // {
    //     dir = dir_open_root();
    // }
    



    // char * slash_indeces[MAX_PATH_DEPTH];
    // int length = strlen(dir);
    // int num_slashes = parse_slashes(dir, slash_indeces);
    // struct thread * t = thread_current();
    // int i;
    // struct dir *curdir = NULL;  

    // /* Absolute path:  A '/' exists in first index */
    // if(num_slashes > 0 && slash_indeces[0] == dir) {
    //     curdir = dir_open_root();
    // } else { /* Relative path: set curdir to where this thread is */
    //     curdir = t->curdir;
    //     if(curdir == NULL) {
    //         curdir = dir_open_root();
    //     }
    // }

    // /* Find which slash ends the last true name. This means like
    // in example "a/b/c/d//////" d would be the last true name. */
    // char * the_end = dir + length - 1;
    // for(i = num_slashes - 1; i >= 0; i++) {
    //     if(slash_indeces[i] == the_end) {
    //         num_slashes--;
    //         the_end--;
    //         length--;
    //     } else {
    //         break;
    //     }
    // }

    // /* Different strategy. Parse up to the next slash. */
    // char name[NAME_MAX + 1];
    // int namelen;
    // char *c = dir;

    // for(i = 0; i < num_slashes; i++) {
    //     /* Get the next path name */
    //     namelen = slash_indeces[i] - c;

    //     /* Make sure we didn't just grab nothing ie two slashes in a row. */
    //     if(namelen <= 0) {
    //         c = slash_indeces[i] + 1;
    //         continue;
    //     }

    //     memcpy(name, c, namelen);
    //     name[namelen] = '\0';

    //     /* Handle the special cases . and .. */
    //     if(!strcmp(name, ".")) {
    //         c = slash_indeces[i] + 1;
    //         continue;
    //     }
    //     if(!strcmp(name, "..")) {
    //         curdir = curdir->parent;
    //         c = slash_indeces[i] + 1;
    //         continue;
    //     }

    //     /* Go look it up, check the directory exists, and store it */
    //     struct inode *inode;
    //     if(dir_lookup(curdir, name, &inode)) {
    //         curdir = dir_open(inode);
    //     } else {
    //         return false;
    //     }
    //     c = slash_indeces[i] + 1;
    // }

    // /* Handle anything after the last '/'. Check that it DOESNT exist */
    // namelen = dir + length - c;
    // if(namelen > 0) {
    //     memcpy(name, c, namelen);
    //     name[namelen] = '\0';
    //     /* Handle the special cases . and .. and note we can't add them */
    //     if(!strcmp(name, ".") || !strcmp(name, "..")) {
    //         return false;
    //     } else {
    //         /* Go look it up, check the directory doesn't exists, and make it */
    //         struct inode *inode;
    //         if(!dir_lookup(curdir, name, &inode)) {
    //             int sector;
    //             return free_map_allocate(1, &sector) && inode_create(sector, 512) && dir_add(curdir, name, sector);
    //         }
    //     }
    // }
    // return false;







    
    // bool success = (dir != NULL &&
    //                 free_map_allocate(1, &inode_sector) &&
    //                 inode_create(inode_sector, initial_size) &&
    //                 dir_add(dir, name, inode_sector));
    // if (!success && inode_sector != 0) 
    //     free_map_release(inode_sector, 1);
    // dir_close(dir);

    return dir_mkdir(name, false, initial_size);
    // return success;
}

/*! Opens the file with the given NAME.  Returns the new file if successful
    or a null pointer otherwise.  Fails if no file named NAME exists,
    or if an internal memory allocation fails. */
struct file * filesys_open(const char *name) {
    /*struct dir *dir = dir_open_root();*/
    struct thread * t = thread_current();
    struct dir * dir = t->curdir;
    struct inode *inode = NULL;

    if (dir == NULL)
    {
        dir = dir_open_root();
    }

    if (dir != NULL)
        dir_lookup(dir, name, &inode);
    dir_close(dir);

    return file_open(inode);
}

/*! Deletes the file named NAME.  Returns true if successful, false on failure.
    Fails if no file named NAME exists, or if an internal memory allocation
    fails. */
bool filesys_remove(const char *name) {
    /*struct dir *dir = dir_open_root();*/
    struct thread * t = thread_current();
    struct dir * dir = t->curdir;

    if (dir == NULL)
    {
        dir = dir_open_root();
    }
    bool success = dir != NULL && dir_remove(dir, name);
    dir_close(dir);

    return success;
}

/*! Formats the file system. */
static void do_format(void) {
    printf("Formatting file system...");
    free_map_create();
    if (!dir_create(ROOT_DIR_SECTOR, 16))
        PANIC("root directory creation failed");
    free_map_close();
    printf("done.\n");
}

