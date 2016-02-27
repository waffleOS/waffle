#ifndef VM_STRUCTS_H
#define VM_STRUCTS_H
/* Page statuses */
enum page_status {
    NEED_FRAME,
    IN_SWAP
};

/* Virtual memory pages. */
struct page {
    struct frame *pframe;
    enum page_status status;
};


/* Physical memory frames. */
struct frame {
    /* Physical address. */
    void *addr;
    /* Page associated with frame. */
    struct page *fpage;
    /* Hash elem to store in frame table. */
    struct hash_elem elem;
};

/* Supplemental page */
struct page_sup {
    struct file * file;
    off_t ofs;
    uint8_t * upage;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
}


#endif /* VM_STRUCTS_H */
