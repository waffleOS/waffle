#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"

int parse_slashes(const char * dir, char * slash_indeces[]);

/*! A directory. */
struct dir {
    struct inode *inode;                /*!< Backing store. */
    off_t pos;                          /*!< Current position. */
    struct dir *parent;             /* Pointer to the parent directory */
};

/*! A single directory entry. */
struct dir_entry {
    block_sector_t inode_sector;        /*!< Sector number of header. */
    char name[NAME_MAX + 1];            /*!< Null terminated file name. */
    bool in_use;                        /*!< In use or free? */
};

/*! Creates a directory with space for ENTRY_CNT entries in the
    given SECTOR.  Returns true if successful, false on failure. */
bool dir_create(block_sector_t sector, size_t entry_cnt) {
    return inode_create(sector, entry_cnt * sizeof(struct dir_entry));
}

/*! Opens and returns the directory for the given INODE, of which
    it takes ownership.  Returns a null pointer on failure. */
struct dir * dir_open(struct inode *inode) {
/*    printf("Dir open?\n");*/
    struct dir *dir = calloc(1, sizeof(*dir));
    if (inode != NULL && dir != NULL) {
        dir->inode = inode;
        dir->pos = 0;
        dir->parent = dir; /* Put something ehre */
        return dir;
    }
    else {
        inode_close(inode);
        free(dir);
        return NULL; 
    }
}

/*! Opens the root directory and returns a directory for it.
    Return true if successful, false on failure. */
struct dir * dir_open_root(void) {
    return dir_open(inode_open(ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir * dir_reopen(struct dir *dir) {
    return dir_open(inode_reopen(dir->inode));
}

/*! Destroys DIR and frees associated resources. */
void dir_close(struct dir *dir) {
    if (dir != NULL) {
        inode_close(dir->inode);
        free(dir);
    }
}

/*! Returns the inode encapsulated by DIR. */
struct inode * dir_get_inode(struct dir *dir) {
    return dir->inode;
}

/*! Searches DIR for a file with the given NAME.
    If successful, returns true, sets *EP to the directory entry
    if EP is non-null, and sets *OFSP to the byte offset of the
    directory entry if OFSP is non-null.
    otherwise, returns false and ignores EP and OFSP. */
static bool lookup(const struct dir *dir, const char *name,
                   struct dir_entry *ep, off_t *ofsp) {
    /*printf("In lookup, finding name: %s\tcurdir: %p\n", name, dir);
    printf("Curdir sector: %d\n", inode_get_inumber(dir->inode));*/
    struct dir_entry e;
    size_t ofs;

    ASSERT(dir != NULL);
    ASSERT(name != NULL);

    for (ofs = 0; inode_read_at(dir->inode, &e, sizeof(e), ofs) == sizeof(e);
         ofs += sizeof(e)) {

        if (e.in_use && !strcmp(name, e.name)) {
            if (ep != NULL)
                *ep = e;
            if (ofsp != NULL)
                *ofsp = ofs;
/*            printf("Found\n");*/
            return true;
        }
    }
/*    printf("Not found\n");*/
    return false;
}

/*! Searches DIR for a file with the given NAME and returns true if one exists,
    false otherwise.  On success, sets *INODE to an inode for the file,
    otherwise to a null pointer.  The caller must close *INODE. */
bool dir_lookup(const struct dir *dir, const char *name, struct inode **inode) {
    struct dir_entry e;

    ASSERT(dir != NULL);
    ASSERT(name != NULL);

    if (lookup(dir, name, &e, NULL))
        *inode = inode_open(e.inode_sector);
    else
        *inode = NULL;

    return *inode != NULL;
}

/*! Adds a file named NAME to DIR, which must not already contain a file by
    that name.  The file's inode is in sector INODE_SECTOR.
    Returns true if successful, false on failure.
    Fails if NAME is invalid (i.e. too long) or a disk or memory
    error occurs. */
bool dir_add(struct dir *dir, const char *name, block_sector_t inode_sector) {
/*    printf("In dir add, name: %s\n", name);*/
    struct dir_entry e;
    off_t ofs;
    bool success = false;

    ASSERT(dir != NULL);
    ASSERT(name != NULL);

    /* Check NAME for validity. */
    if (*name == '\0' || strlen(name) > NAME_MAX)
        return false;
    // printf("NAME IN USE?\n", name);

    /* Check that NAME is not in use. */
    if (lookup(dir, name, NULL, NULL)) {
        // printf("YOU BUTT\n");
        goto done;
    }
    // printf("NOODLES AND POODLES\n", name);

    /* Set OFS to offset of free slot.
       If there are no free slots, then it will be set to the
       current end-of-file.
     
       inode_read_at() will only return a short read at end of file.
       Otherwise, we'd need to verify that we didn't get a short
       read due to something intermittent such as low memory. */
    for (ofs = 0; inode_read_at(dir->inode, &e, sizeof(e), ofs) == sizeof(e);
         ofs += sizeof(e)) {
        if (!e.in_use)
            break;
    }

    /* Write slot. */
    e.in_use = true;
    strlcpy(e.name, name, sizeof e.name);
    e.inode_sector = inode_sector;
    success = inode_write_at(dir->inode, &e, sizeof(e), ofs) == sizeof(e);

done:
    return success;
}

/*! Removes any entry for NAME in DIR.  Returns true if successful, false on
    failure, which occurs only if there is no file with the given NAME. */
bool dir_remove(struct dir *dir, const char *name) {
    struct dir_entry e;
    struct inode *inode = NULL;
    bool success = false;
    off_t ofs;

    ASSERT(dir != NULL);
    ASSERT(name != NULL);

    /* Find directory entry. */
    if (!lookup(dir, name, &e, &ofs))
        goto done;

    /* Open inode. */
    inode = inode_open(e.inode_sector);
    if (inode == NULL)
        goto done;

    /* Erase directory entry. */
    e.in_use = false;
    if (inode_write_at(dir->inode, &e, sizeof(e), ofs) != sizeof(e))
        goto done;

    /* Remove inode. */
    inode_remove(inode);
    success = true;

done:
    inode_close(inode);
    return success;
}

/*! Reads the next directory entry in DIR and stores the name in NAME.  Returns
    true if successful, false if the directory contains no more entries. */
bool dir_readdir(struct dir *dir, char name[NAME_MAX + 1]) {
    struct dir_entry e;

    while (inode_read_at(dir->inode, &e, sizeof(e), dir->pos) == sizeof(e)) {
        dir->pos += sizeof(e);
        if (e.in_use) {
            strlcpy(name, e.name, NAME_MAX + 1);
            return true;
        } 
    }
    return false;
}


bool dir_chdir(const char *dir) {
    char * slash_indeces[MAX_PATH_DEPTH];
    int length = strlen(dir);
    int num_slashes = parse_slashes(dir, slash_indeces);
    struct thread * t = thread_current();
    struct dir *curdir = NULL;

    /* Absolute path:  A '/' exists in first index */
    if(num_slashes > 0 && slash_indeces[0] == dir) {
        curdir = dir_open_root();
    } else { /* Relative path: set curdir to where this thread is */
/*        printf("Opening a relative path\n");*/
        curdir = t->curdir;
/*        printf("curdir: %p\n", curdir);*/
        if (curdir != NULL)
        {
/*            printf("Curdir sector: %d\n", inode_get_inumber(curdir->inode));*/
        }
        /*printf("RELATIVELYSPEAKING curdir = %d\n", curdir);*/
        if(curdir == NULL) {
            /*printf("ITS NULLLLLLL\n");*/
            curdir = dir_open_root();
        }
    }

/*    printf("In chdir, changing dir to: %s\n\tcurdir:%p\n", dir, curdir);*/

    /* Different strategy. Parse up to the next slash. */
    int i;
    char *c = dir;
    char name[NAME_MAX + 1];
    int namelen;

    for(i = 0; i < num_slashes; i++) {
        /* Get the next path name */
        namelen = slash_indeces[i] - c;

        /* Make sure we didn't just grab nothing ie two slashes in a row. */
        if(namelen <= 0) {
            c = slash_indeces[i] + 1;
            continue;
        }

        memcpy(name, c, namelen);
        name[namelen] = '\0';

        /* Handle the special cases . and .. */
        if(!strcmp(name, ".")) {
            c = slash_indeces[i] + 1;
            continue;
        }
        if(!strcmp(name, "..")) {
            curdir = curdir->parent;
            c = slash_indeces[i] + 1;
            continue;
        }

        /* Go look it up, check the directory exists, and store it */
        struct inode *inode;
        if(dir_lookup(curdir, name, &inode)) {
            curdir = dir_open(inode);
        } else {
            return false;
        }
        c = slash_indeces[i] + 1;

    }

    /*printf("CHDIR: AM I HERE?\n");*/
    /* Handle anything after the last '/'. Check that it DOESNT exist */
    namelen = dir + length - c;
    if(namelen > 0) {
        memcpy(name, c, namelen);
        name[namelen] = '\0';
        /* Handle the special cases . and .. and note we can't add them */
        if(!strcmp(name, ".")) {
            /* Do nothing */
        } else if (!strcmp(name, "..")) {
            curdir = curdir->parent;
            return false;
        } else {
            /* Go look it up, check the directory exists */
            /*printf("CHDIR: LOOKUP AND CHECK\n");*/
            struct inode *inode;
            // printf("In chdir, looking up name: %s\n", name);
            if(dir_lookup(curdir, name, &inode)) {
                // int sector;
                // printf("CHDIR: IN THE LOOKUP name = %s, curdir = %d\n", name, curdir);
                // bool success = free_map_allocate(1, &sector);
                // printf("sucess1 = %d\n", success);
                // success = success && inode_create(sector, 512);
                // printf("sucess2 = %d\n", success);
                // success = success && dir_add(curdir, name, sector);
                // printf("sucess3 = %d\n", success);
                // curdir = dir_open(inode);
                // return success;
                curdir = dir_open(inode);

/*                printf("Opened name: %s\tcurdir: %p\n", name, curdir);
                printf("Curdir sector: %d\n", inode_get_inumber(curdir->inode));*/
                t->curdir = curdir;
                return true;
                // return success;

            }
        }
    }

    // for(i = 0; i < num_slashes; i++) {
    //     /* Get the next path name */
    //     namelen = slash_indeces[i] - c;

    //     /* Make sure we didn't just grab nothing ie two slashes in a row. */
    //     if(namelen <= 0) {
    //         c = slash_indeces[i] + 1;
    //         continue;
    //     }

    //     strlcpy(name, c, namelen);
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
    //     struct inode **inode;
    //     if(dir_lookup(curdir, name, inode)) {
    //         curdir = dir_open(*inode);
    //     } else {
    //         return false;
    //     }
    //     c = slash_indeces[i] + 1;
    // }


    // /* Handle anything after the last '/' */
    // namelen = dir + length - c;
    // if(namelen > 0) {
    //     strlcpy(name, c, namelen);
    //     name[namelen] = '\0';

    //     /* Handle the special cases . and .. */
    //     if(!strcmp(name, ".")) {
    //         /* Do nothing */
    //     } else if(!strcmp(name, "..")) {
    //         curdir = curdir->parent;
    //     } else {
    //         /* Go look it up, check the directory exists, and store it */
    //         struct inode **inode;
    //         if(dir_lookup(curdir, name, inode)) {
    //             curdir = dir_open(*inode);
    //         } else {
    //             return false;
    //         }
    //     }
    // }


    // /******* Corner case: Process up to the first slash ***/
    // // process_next_name(name, dir, slash_indeces)
    // char name[NAME_MAX + 1];
    // int namelen;

    // /* Get the next path name */
    // if(num_slashes == 0) { /* get everything to the end of dir */
    //     namelen = length;
    // } else { /* Somewhere in the middle */
    //     namelen = slash_indeces[0] - dir;
    // }

    // if(namelen > 0) {
    //     strlcpy(name, dir, namelen);
    //     name[namelen] = '\0';
    // }

    // /* Handle the special cases . and .. */
    // if(!strcmp(name, ".")) {
    //     /* Do nothing */
    // } else if(!strcmp(name, "..")) {
    //     curdir = curdir->parent;
    // } else {
    //     /* Go look it up, check the directory exists, and store it */
    //     struct inode **inode;
    //     if(dir_lookup(curdir, name, inode)) {
    //         curdir = dir_open(*inode);
    //     } else {
    //         return false;
    //     }
    // }

    // /******** Process everything from first slash on *****/
    // int i;
    // for(i = 0; i < num_slashes; i++) {
    //     // char name[NAME_MAX + 1];
    //     // int namelen;

    //     /* Get the next path name */
    //     if(i == num_slashes - 1) { /* get everything to the end of dir */
    //         namelen = dir + length - (slash_indeces[i] + 1);
    //     } else { /* Somewhere in the middle */
    //         namelen = slash_indeces[i + 1] - (slash_indeces[i] + 1);
    //     }

    //     /* Make sure we didn't just grab nothing. */
    //     if(namelen <= 0) {
    //         continue;
    //     }

    //     strncpy(name, slash_indeces[i] + 1, namelen);
    //     name[namelen] = '\0';

    //     /* Handle the special cases . and .. */
    //     if(!strcmp(name, ".")) {
    //         continue;
    //     }
    //     if(!strcmp(name, "..")) {
    //         curdir = curdir->parent;
    //         continue;
    //     }

    //     /* Go look it up, check the directory exists, and store it */
    //     struct inode **inode;
    //     if(dir_lookup(curdir, name, inode)) {
    //         curdir = dir_open(*inode);
    //     } else {
    //         return false;
    //     }
    // }



    /* If everything worked out correctly, set this thread to that
    current directory we found */
    // t->curdir = curdir;    

    // return true;
    return false;
}

bool dir_mkdir(const char *dir, bool isDirectory, off_t file_size) {
/*    printf("MKDIR dir = %s\n", dir);
*/

    char * slash_indeces[MAX_PATH_DEPTH];
    int length = strlen(dir);
    int num_slashes = parse_slashes(dir, slash_indeces);
    struct thread * t = thread_current();
    int i;
    struct dir *curdir = NULL;  

  /*printf("WHEREAMI curdir = %d\n", curdir);*/
    /* Absolute path:  A '/' exists in first index */
    if(num_slashes > 0 && slash_indeces[0] == dir) {
  /*printf("ABOSOLUTELY\n");*/
        curdir = dir_open_root();
    } else { /* Relative path: set curdir to where this thread is */
        curdir = t->curdir;
/*  printf("RELATIVELYSPEAKING curdir = %d\n", curdir);*/
        if(curdir == NULL) {
/*            printf("ITS NULLLLLLL\n");*/
            curdir = dir_open_root();
        }
    }
/*  printf("IM OUT curdir = %d\n", curdir);*/

    /* Find which slash ends the last true name. This means like
    in example "a/b/c/d//////" d would be the last true name. */
    char * the_end = dir + length - 1;
    for(i = num_slashes - 1; i >= 0; i++) {
  /*printf("THE PRE FOR LOOOOOOOOOOOOOOOOOOOOPY\n");*/
        if(slash_indeces[i] == the_end) {
            num_slashes--;
            the_end--;
            length--;
        } else {
            break;
        }
    }
  /*printf("HEREE YOU SAY????????\n");*/


    /* Different strategy. Parse up to the next slash. */
    char name[NAME_MAX + 1];
    int namelen;
    char *c = dir;

    for(i = 0; i < num_slashes; i++) {
/*  printf("LOOOOPOPPPITTTTTTTTTTTTTTTTTTTTTTTT\n");*/
        /* Get the next path name */
        namelen = slash_indeces[i] - c;

        /* Make sure we didn't just grab nothing ie two slashes in a row. */
        if(namelen <= 0) {
            c = slash_indeces[i] + 1;
/*            printf("YEAH YOU SHOULAD GOTTEN HERE c = %s\n", c);*/
            continue;
        }

/*        printf("I'm looking for c again = %s\n", c);*/
        memcpy(name, c, namelen);
        name[namelen] = '\0';

        /* Handle the special cases . and .. */
        if(!strcmp(name, ".")) {
            c = slash_indeces[i] + 1;
            continue;
        }
        if(!strcmp(name, "..")) {
            curdir = curdir->parent;
            c = slash_indeces[i] + 1;
            continue;
        }

        /* Go look it up, check the directory exists, and store it */
        struct inode *inode;
/*        printf("I'm looking for c again2 = %s, name = %s\n", c, name);*/
        if(dir_lookup(curdir, name, &inode)) {
            curdir = dir_open(inode);
        } else {
            return false;
        }
        c = slash_indeces[i] + 1;

    }

/*printf("GOLDLLDLDELENENNENENNNENENNENENAU79\n");*/
    /* Handle anything after the last '/'. Check that it DOESNT exist */
    namelen = dir + length - c;
    if(namelen > 0) {
        memcpy(name, c, namelen);
        name[namelen] = '\0';
/*printf("name = %s, c = %s, namelen = %d, name[0] = %c\n", name, c, namelen, name[0]);*/
        /* Handle the special cases . and .. and note we can't add them */
        if(!strcmp(name, ".") || !strcmp(name, "..")) {
            return false;
        } else {
            /* Go look it up, check the directory doesn't exists, and make it */
/*printf("IN ABOUT TO LOOKIT UPETET\n");*/
            struct inode *inode;
/*printf("IN THE LOOOOOOOOOOOOOOOOOOOOOOOOOKINGEMENTETETETETET\n");*/
/*printf("dir = %d, name = %s, inode = %d\n", curdir, name, inode);*/
            if(!dir_lookup(curdir, name, &inode)) {
                /*return dir_add(curdir, name, inode_get_inumber(inode));*/
                int sector = 0;
/*                printf("Creating dir %s\n", name);*/
                bool success = free_map_allocate(1, &sector) && inode_create(sector, file_size) && dir_add(curdir, name, sector);
                if (!success && sector != 0) 
                    free_map_release(sector, 1);
                // dir_close(dir);
                    /* Set the isdirectory flag */
                if(isDirectory) {
                    inode_set_dir(sector, isDirectory);
                }

                return success;

            }
        }
    }



/*printf("GOLDLLDLDFALSE\n");*/
    return false;






    // /* Absolute path:  A '/' exists in first index */
    // if(num_slashes > 0 && slash_indeces[0] == dir) {
    //     curdir = dir_open_root();
    // } else { /* Relative path: set curdir to where this thread is */
    //     curdir = t->curdir;
    // }

    // int i;
    // for(i = 0; i < num_slashes - 1; i++) {
    //     char name[NAME_MAX + 1];
    //     int namelen;

    //     /* Get the next path name */
    //     if(i == num_slashes - 1) { /* get everything to the end of dir */
    //         namelen = dir + length - (slash_indeces[i] + 1);
    //     } else { /* Somewhere in the middle */
    //         namelen = slash_indeces[i + 1] - (slash_indeces[i] + 1);
    //     }

    //     /* Make sure we didn't just grab nothing. */
    //     if(namelen <= 0) {
    //         continue;
    //     }

    //     strncpy(name, slash_indeces[i] + 1, namelen);
    //     name[namelen] = '\0';

    //     /* Handle the special cases . and .. */
    //     if(!strcmp(name, ".")) {
    //         continue;
    //     }
    //     if(!strcmp(name, "..")) {
    //         curdir = curdir->parent;
    //         continue;
    //     }

    //     /* Go look it up, check the directory exists, and store it */
    //     struct inode **inode;
    //     if(dir_lookup(curdir, name, inode)) {
    //         curdir = dir_open(*inode);
    //     } else {
    //         return false;
    //     }
    // }



    // /* If everything worked out correctly, set this thread to that
    // current directory we found */
    // t->curdir = curdir;    

    return true;
    return false;
}

/* Fills array slash_indeces with the pointers to the slashes. Returns
the number of slashes found */
int parse_slashes(const char * dir, char * slash_indeces[]) {
    int i, ind = 0;
    /*printf("IN PARSELSLASHES dir = %s\n", dir);*/
    /* Fill it with zeroes */
    for(i = 0; dir[i] != '\0'; i++) {
        slash_indeces[i] = 0;
    }

    /* fill slash_indeces with pointers to the slashes */
    for(i = 0; dir[i] != '\0'; i++) {
        if(dir[i] == '/') {
            /*printf("PARSE_SLASHES I FOUND ONE\n");*/
            if(ind < MAX_PATH_DEPTH) {
                slash_indeces[ind] = dir + i;
                ind++;
            } else {
                /* Too many slashes */
                do_exit(-1);
            }
        }
    }
    return ind;
}

bool dir_open_file(const char *dir, struct inode **inode_ptr) {
    // printf("MKDIR dir = %s\n", dir);


    char * slash_indeces[MAX_PATH_DEPTH];
    int length = strlen(dir);
    int num_slashes = parse_slashes(dir, slash_indeces);
    struct thread * t = thread_current();
    int i;
    struct dir *curdir = NULL;  

  /*printf("WHEREAMI curdir = %d\n", curdir);*/
    /* Absolute path:  A '/' exists in first index */
    if(num_slashes > 0 && slash_indeces[0] == dir) {
  /*printf("ABOSOLUTELY\n");*/
        curdir = dir_open_root();
    } else { /* Relative path: set curdir to where this thread is */
        curdir = t->curdir;
/*  printf("RELATIVELYSPEAKING curdir = %d\n", curdir);*/
        if(curdir == NULL) {
/*            printf("ITS NULLLLLLL\n");*/
            curdir = dir_open_root();
        }
    }
/*  printf("IM OUT curdir = %d\n", curdir);*/

    /* Find which slash ends the last true name. This means like
    in example "a/b/c/d//////" d would be the last true name. */
    char * the_end = dir + length - 1;
    for(i = num_slashes - 1; i >= 0; i++) {
  /*printf("THE PRE FOR LOOOOOOOOOOOOOOOOOOOOPY\n");*/
        if(slash_indeces[i] == the_end) {
            num_slashes--;
            the_end--;
            length--;
        } else {
            break;
        }
    }
  /*printf("HEREE YOU SAY????????\n");*/


    /* Different strategy. Parse up to the next slash. */
    char name[NAME_MAX + 1];
    int namelen;
    char *c = dir;

    for(i = 0; i < num_slashes; i++) {
/*  printf("LOOOOPOPPPITTTTTTTTTTTTTTTTTTTTTTTT\n");*/
        /* Get the next path name */
        namelen = slash_indeces[i] - c;

        /* Make sure we didn't just grab nothing ie two slashes in a row. */
        if(namelen <= 0) {
            c = slash_indeces[i] + 1;
/*            printf("YEAH YOU SHOULAD GOTTEN HERE c = %s\n", c);*/
            continue;
        }

/*        printf("I'm looking for c again = %s\n", c);*/
        memcpy(name, c, namelen);
        name[namelen] = '\0';

        /* Handle the special cases . and .. */
        if(!strcmp(name, ".")) {
            c = slash_indeces[i] + 1;
            continue;
        }
        if(!strcmp(name, "..")) {
            curdir = curdir->parent;
            c = slash_indeces[i] + 1;
            continue;
        }

        /* Go look it up, check the directory exists, and store it */
        struct inode *inode;
/*        printf("I'm looking for c again2 = %s, name = %s\n", c, name);*/
        if(dir_lookup(curdir, name, &inode)) {
            curdir = dir_open(inode);
        } else {
            return false;
        }
        c = slash_indeces[i] + 1;

    }

/*printf("GOLDLLDLDELENENNENENNNENENNENENAU79\n");*/
    /* Handle anything after the last '/'. Check that it DOESNT exist */
    namelen = dir + length - c;
    if(namelen > 0) {
        memcpy(name, c, namelen);
        name[namelen] = '\0';
/*printf("name = %s, c = %s, namelen = %d, name[0] = %c\n", name, c, namelen, name[0]);*/
        /* Handle the special cases . and .. and note we can't add them */
        if(!strcmp(name, ".") || !strcmp(name, "..")) {
            return false;
        } else {
            /* Go look it up, check the directory doesn't exists, and make it */
            // printf("curdir = %p, name = %s, inode_ptr = %p\n", curdir, name, inode_ptr);
            if(dir_lookup(curdir, name, inode_ptr)) {
                // printf("inode_ptr = %p\n", inode_ptr);
                return true;

            }
        }
    }
    return false;

}


bool dir_rmdir(const char *dir) {
/*    printf("MKDIR dir = %s\n", dir);
*/

    char * slash_indeces[MAX_PATH_DEPTH];
    int length = strlen(dir);
    int num_slashes = parse_slashes(dir, slash_indeces);
    struct thread * t = thread_current();
    int i;
    struct dir *curdir = NULL;  

  /*printf("WHEREAMI curdir = %d\n", curdir);*/
    /* Absolute path:  A '/' exists in first index */
    if(num_slashes > 0 && slash_indeces[0] == dir) {
  /*printf("ABOSOLUTELY\n");*/
        curdir = dir_open_root();
    } else { /* Relative path: set curdir to where this thread is */
        curdir = t->curdir;
/*  printf("RELATIVELYSPEAKING curdir = %d\n", curdir);*/
        if(curdir == NULL) {
/*            printf("ITS NULLLLLLL\n");*/
            curdir = dir_open_root();
        }
    }
/*  printf("IM OUT curdir = %d\n", curdir);*/

    /* Find which slash ends the last true name. This means like
    in example "a/b/c/d//////" d would be the last true name. */
    char * the_end = dir + length - 1;
    for(i = num_slashes - 1; i >= 0; i++) {
  /*printf("THE PRE FOR LOOOOOOOOOOOOOOOOOOOOPY\n");*/
        if(slash_indeces[i] == the_end) {
            num_slashes--;
            the_end--;
            length--;
        } else {
            break;
        }
    }
  /*printf("HEREE YOU SAY????????\n");*/


    /* Different strategy. Parse up to the next slash. */
    char name[NAME_MAX + 1];
    int namelen;
    char *c = dir;

    for(i = 0; i < num_slashes; i++) {
/*  printf("LOOOOPOPPPITTTTTTTTTTTTTTTTTTTTTTTT\n");*/
        /* Get the next path name */
        namelen = slash_indeces[i] - c;

        /* Make sure we didn't just grab nothing ie two slashes in a row. */
        if(namelen <= 0) {
            c = slash_indeces[i] + 1;
/*            printf("YEAH YOU SHOULAD GOTTEN HERE c = %s\n", c);*/
            continue;
        }

/*        printf("I'm looking for c again = %s\n", c);*/
        memcpy(name, c, namelen);
        name[namelen] = '\0';

        /* Handle the special cases . and .. */
        if(!strcmp(name, ".")) {
            c = slash_indeces[i] + 1;
            continue;
        }
        if(!strcmp(name, "..")) {
            curdir = curdir->parent;
            c = slash_indeces[i] + 1;
            continue;
        }

        /* Go look it up, check the directory exists, and store it */
        struct inode *inode;
/*        printf("I'm looking for c again2 = %s, name = %s\n", c, name);*/
        if(dir_lookup(curdir, name, &inode)) {
            curdir = dir_open(inode);
        } else {
            return false;
        }
        c = slash_indeces[i] + 1;

    }

/*printf("GOLDLLDLDELENENNENENNNENENNENENAU79\n");*/
    /* Handle anything after the last '/'. Check that it DOESNT exist */
    namelen = dir + length - c;
    if(namelen > 0) {
        memcpy(name, c, namelen);
        name[namelen] = '\0';
/*printf("name = %s, c = %s, namelen = %d, name[0] = %c\n", name, c, namelen, name[0]);*/
        /* Handle the special cases . and .. and note we can't add them */
        if(!strcmp(name, ".") || !strcmp(name, "..")) {
            return false;
        } else {
            struct inode *inode;
            /* Go look it up, check the directory doesn't exists, and make it */
/*            printf("curdir = %p, name = %s, inode = %p\n", curdir, name, inode);*/
            if(dir_lookup(curdir, name, &inode)) {
                bool success = dir_remove(curdir, name);
                dir_close(curdir);
/*                printf("success = %d\n", success);*/
                return success;
            }
        }
    }
    return false;

}



