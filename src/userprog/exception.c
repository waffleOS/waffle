#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "vm/page.h"
#include "vm/frame_table.h"
#include "vm/swap.h"
#include "threads/vaddr.h"

/*! Number of page faults processed. */
static long long page_fault_cnt;

static void kill(struct intr_frame *);
static void page_fault(struct intr_frame *);


/*! Registers handlers for interrupts that can be caused by user programs.

    In a real Unix-like OS, most of these interrupts would be passed along to
    the user process in the form of signals, as described in [SV-386] 3-24 and
    3-25, but we don't implement signals.  Instead, we'll make them simply kill
    the user process.

    Page faults are an exception.  Here they are treated the same way as other
    exceptions, but this will need to change to implement virtual memory.

    Refer to [IA32-v3a] section 5.15 "Exception and Interrupt Reference" for a
    description of each of these exceptions. */
void exception_init(void) {
    /* These exceptions can be raised explicitly by a user program,
       e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
       we set DPL==3, meaning that user programs are allowed to
       invoke them via these instructions. */
    intr_register_int(3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
    intr_register_int(4, 3, INTR_ON, kill, "#OF Overflow Exception");
    intr_register_int(5, 3, INTR_ON, kill,
                      "#BR BOUND Range Exceeded Exception");

    /* These exceptions have DPL==0, preventing user processes from
       invoking them via the INT instruction.  They can still be
       caused indirectly, e.g. #DE can be caused by dividing by
       0.  */
    intr_register_int(0, 0, INTR_ON, kill, "#DE Divide Error");
    intr_register_int(1, 0, INTR_ON, kill, "#DB Debug Exception");
    intr_register_int(6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
    intr_register_int(7, 0, INTR_ON, kill,
                      "#NM Device Not Available Exception");
    intr_register_int(11, 0, INTR_ON, kill, "#NP Segment Not Present");
    intr_register_int(12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
    intr_register_int(13, 0, INTR_ON, kill, "#GP General Protection Exception");
    intr_register_int(16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
    intr_register_int(19, 0, INTR_ON, kill,
                      "#XF SIMD Floating-Point Exception");

    /* Most exceptions can be handled with interrupts turned on.
       We need to disable interrupts for page faults because the
       fault address is stored in CR2 and needs to be preserved. */
    intr_register_int(14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/*! Prints exception statistics. */
void exception_print_stats(void) {
    printf("Exception: %lld page faults\n", page_fault_cnt);
}

/*! Handler for an exception (probably) caused by a user process. */
static void kill(struct intr_frame *f) {
    /* This interrupt is one (probably) caused by a user process.
       For example, the process might have tried to access unmapped
       virtual memory (a page fault).  For now, we simply kill the
       user process.  Later, we'll want to handle page faults in
       the kernel.  Real Unix-like operating systems pass most
       exceptions back to the process via signals, but we don't
       implement them. */
     
    /* The interrupt frame's code segment value tells us where the
       exception originated. */
    switch (f->cs) {
    case SEL_UCSEG:
        /* User's code segment, so it's a user exception, as we
           expected.  Kill the user process.  */
        printf("%s: dying due to interrupt %#04x (%s).\n",
               thread_name(), f->vec_no, intr_name(f->vec_no));
        intr_dump_frame(f);

        /* Implemented for waiting. Set exit status to -1 */
        thread_current()->exit_status = -1;
        do_exit(-1);

    case SEL_KCSEG:
        /* Kernel's code segment, which indicates a kernel bug.
           Kernel code shouldn't throw exceptions.  (Page faults
           may cause kernel exceptions--but they shouldn't arrive
           here.)  Panic the kernel to make the point.  */
        intr_dump_frame(f);
        PANIC("Kernel bug - unexpected interrupt in kernel"); 

    default:
        /* Some other code segment?  Shouldn't happen.  Panic the
           kernel. */
        printf("Interrupt %#04x (%s) in unknown segment %04x\n",
               f->vec_no, intr_name(f->vec_no), f->cs);
        thread_current()->exit_status = -1;
        thread_exit();
    }
}

/*! Page fault handler.  This is a skeleton that must be filled in
    to implement virtual memory.  Some solutions to project 2 may
    also require modifying this code.

    At entry, the address that faulted is in CR2 (Control Register
    2) and information about the fault, formatted as described in
    the PF_* macros in exception.h, is in F's error_code member.  The
    example code here shows how to parse that information.  You
    can find more information about both of these in the
    description of "Interrupt 14--Page Fault Exception (#PF)" in
    [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void page_fault(struct intr_frame *f) {
    bool not_present;  /* True: not-present page, false: writing r/o page. */
    bool write;        /* True: access was write, false: access was read. */
    bool user;         /* True: access by user, false: access by kernel. */
    void *fault_addr;  /* Fault address. */

    /* Obtain faulting address, the virtual address that was accessed to cause
       the fault.  It may point to code or to data.  It is not necessarily the
       address of the instruction that caused the fault (that's f->eip).
       See [IA32-v2a] "MOV--Move to/from Control Registers" and
       [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception (#PF)". */
    asm ("movl %%cr2, %0" : "=r" (fault_addr));

    /* Turn interrupts back on (they were only off so that we could
       be assured of reading CR2 before it changed). */
    intr_enable();

    /* Count page faults. */
    page_fault_cnt++;

    /* Determine cause. */
    not_present = (f->error_code & PF_P) == 0;
    write = (f->error_code & PF_W) != 0;
    user = (f->error_code & PF_U) != 0;

    /* To implement virtual memory, delete the rest of the function
       body, and replace it with code that brings in the page to
       which fault_addr refers. */
    
    /* First, check if fault_addr corresponds to a page_info in the
     * sup_page_table. If it does, then the process is demanding that
     * page and we need to falloc it. */
    struct thread * t = thread_current(); 
    struct page_info * page_info = page_info_lookup(&t->sup_page_table, (uint8_t *) fault_addr); 
    
    /* If no page_info, we check to see if it's a stack access. */
    if (page_info == NULL)
    {
        /* Look at the frame's esp. */
        uint8_t * esp = (uint8_t *) f->esp;
        if (f->cs == SEL_KCSEG)
        {
            /*printf("Old ESP: %p\n", esp);*/
            /*printf("New ESP: %p\n", t->esp);*/
            /*printf("Fault addr: %p\n", fault_addr);*/
            esp = t->esp;
        }
        /* This is a heuristic for determining a stack access. */
        if ((uint8_t *) fault_addr >= esp - 64 && (uint8_t *) fault_addr < (uint8_t *) PHYS_BASE)
        {
                struct page_info * page_info = install_page_info(fault_addr, NULL, 0, 0, 0, false, STACK);
                struct frame * frame = falloc(page_info);
                uint8_t * kpage = frame->addr;
                if(!install_page(pg_round_down(fault_addr), kpage, true))
                {
                   free_frame(frame); 
                }
                t->esp = fault_addr;

        }
        /* Otherwise, this is a "real" page fault. */
        else
        {
            /*printf("Page fault at %p: %s error %s page in %s context.\n",*/
                   /*fault_addr,*/
                   /*not_present ? "not present" : "rights violation",*/
                   /*write ? "writing" : "reading",*/
                   /*user ? "user" : "kernel");*/
            if (lock_held_by_current_thread(&file_lock))
            {
                lock_release(&file_lock);
            }
            do_exit(-1);
        }
    }
    /* Otherwise, the process is demanding a frame. */
    else
    {
        enum page_status status = page_info->status;
        uint8_t * kpage;
        struct frame * frame;
        int read_bytes;
        struct file * file;

        /* page_info->status gives the reason for the fault. */
        switch (status)
        {
            /* Loading executable memory. */
            case LOAD_FILE:
                /* Go to location in file. */
                file_seek(page_info->file, page_info->ofs);
                /* Get a frame for the address. */
                frame = falloc(page_info);
                kpage = frame->addr;
                /* Associate the frame and page in the page table. */
                if (!install_page(page_info->upage, kpage, page_info->writable))
                {
                    free_frame(frame);
                    if (lock_held_by_current_thread(&file_lock))
                    {
                        lock_release(&file_lock);
                    }
                    do_exit(-1);
                    return;
                }
                read_bytes = file_read(page_info->file, kpage, page_info->read_bytes);
                if (read_bytes != (int) page_info->read_bytes) {
                    free_frame(frame);
                    return;
                }
                // printf("Data loaded upage %p \n", page_info->upage);
                /*if (file_read(page_info->file, page_info->upage, page_info->read_bytes) != (int) page_info->read_bytes) {*/
                    /*free_frame(f);*/
                    /*return;*/
                /*}*/
                /* Clear out the remaining memory to avoid leaking previous data. */
                memset(kpage + page_info->read_bytes, 0, page_info->zero_bytes);

                break;
            /* mmap access of a file. */
            case MMAP_FILE:
                file = file_reopen(page_info->file);
                file_seek(file, page_info->ofs);
                frame = falloc(page_info);
                kpage = frame->addr;
                if (!install_page(page_info->upage, kpage, page_info->writable))
                {
                    free_frame(frame);
                    kill(f);
                    return;
                }
                read_bytes = file_read(file, kpage, page_info->read_bytes);
                if (read_bytes != (int) page_info->read_bytes) {
                    free_frame(frame);
                    return;
                }
                /*if (file_read(page_info->file, page_info->upage, page_info->read_bytes) != (int) page_info->read_bytes) {*/
                    /*free_frame(f);*/
                    /*return;*/
                /*}*/
                memset(kpage + page_info->read_bytes, 0, page_info->zero_bytes);
                file_close(file);
                break;
            /* Anonymous file. */
            case ANON_FILE:
                break;
            /* Accessing data stored in swap. */
            case SWAP:
                printf("Restoring from swap\n");
                
                /* Get frame and install in page table. */
                frame = falloc(page_info);
                kpage = frame->addr;
                if (!install_page(page_info->upage, kpage, page_info->writable))
                {
                    printf("Install page failed.\n");
                    free_frame(frame);
                    kill(f);
                    return;
                }
                else {
                    page_info->frame = frame;
                    restore_page(page_info);
                }
                break;
            /* Accessing the stack. */
            case STACK:
                /* Get frame and install in page table. */
                frame = falloc(page_info);
                kpage = frame->addr;
                if(!install_page(((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true))
                {
                   free_frame(frame); 
                   kill(f);
                }
                break;
        }

    }
}

