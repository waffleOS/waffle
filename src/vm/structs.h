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
};



#endif /* VM_STRUCTS_H */
