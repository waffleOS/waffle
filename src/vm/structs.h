#ifndef VM_STRUCTS_H
#define VM_STRUCTS_H
/* Virtual memory pages. */
struct page {
};


/* Physical memory frames. */
struct frame {
    /* Physical address. */
    void *addr;
    /* Page associated with frame. */
    page *fpage;
};



#endif /* VM_STRUCTS_H */
