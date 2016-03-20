#ifndef FILESYS_DIRECTORY_H
#define FILESYS_DIRECTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "devices/block.h"
#include "filesys/off_t.h"


/*! Maximum length of a file name component.
    This is the traditional UNIX maximum length.
    After directories are implemented, this maximum length may be
    retained, but much longer full path names must be allowed. */
#define NAME_MAX 14
/* MAX_PATH_DEPTH determines the number of directories chained together
in a path allowed. More specifically, it is the number of slashes
allowed because this will create an array to store pointers to the
slashes in a given path. */
#define MAX_PATH_DEPTH 100

struct inode;

/* Opening and closing directories. */
bool dir_create(block_sector_t sector, size_t entry_cnt);
struct dir *dir_open(struct inode *);
struct dir *dir_open_root(void);
struct dir *dir_reopen(struct dir *);
void dir_close(struct dir *);
struct inode *dir_get_inode(struct dir *);

bool dir_chdir(const char *dir);
bool dir_mkdir(const char *dir, bool isDirectory, off_t file_size);
bool dir_open_file(const char *dir, struct inode **inode_ptr);
bool dir_rmdir(const char *dir);

/* Reading and writing. */
bool dir_lookup(const struct dir *, const char *name, struct inode **);
bool dir_add(struct dir *, const char *name, block_sector_t);
bool dir_remove(struct dir *, const char *name);
bool dir_readdir(struct dir *, char name[NAME_MAX + 1]);

#endif /* filesys/directory.h */

