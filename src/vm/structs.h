#ifndef VM_STRUCTS_H
#define VM_STRUCTS_H
#include <list.h>
#include "filesys/off_t.h"

/* Page statuses */
enum page_status {
    LOAD_FILE,
    MMAP_FILE,
    ANON_FILE,
    SWAP
};

/* Physical memory frames. */
struct frame {
    /* Physical address. */
    void *addr;
    /* List elem to store in frame table. */
    struct list_elem elem;
    /* Associated page info. */
    struct page_info *pinfo;
};

/* Supplemental page */
struct page_info {
    struct file *file;
    off_t ofs;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
    enum page_status status;
    struct frame *frame;
};


#endif /* VM_STRUCTS_H */
