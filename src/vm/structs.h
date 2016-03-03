#ifndef VM_STRUCTS_H
#define VM_STRUCTS_H
#include <list.h>
#include "filesys/off_t.h"
#include <stdbool.h>
#include <stdint.h>
#include <hash.h>

/* Page statuses */
enum page_status {
    LOAD_FILE,
    MMAP_FILE,
    ANON_FILE,
    SWAP,
    STACK
};

/* Physical memory frames. */
struct frame {
    /* Physical address. */
    void *addr;
    /* List elem to store in frame table. */
    struct list_elem elem;
    /* Associated page info. */
    struct page_info *pinfo;
    /* Age for aging policy. */ 
    uint32_t age;
};

/* Supplemental page */
struct page_info {
    uintptr_t page_num;
    uint8_t * upage;
    struct file *file;
    off_t ofs;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
    enum page_status status;
    struct hash_elem elem;
    struct frame *frame;
    size_t swap_index;
};

/* Mapping */
struct mapping {
    uint8_t * upage;
    struct file * file;
    unsigned int num_pages;
};


#endif /* VM_STRUCTS_H */
