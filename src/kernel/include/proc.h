#ifndef __KERNEL_INCLUDE_PROC__
#define __KERNEL_INCLUDE_PROC__


#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/proc.h"
#include "kernel/include/atomic.h"
#include "kernel/include/struct.h"


// /*
//  * IPC
//  */
// struct msg_node {
//     struct {
//         ulong mailbox_id;
//         struct process *proc;
//         struct thread *thread;
//     } src;
//
//     struct {
//         ulong mailbox_id;
//         struct process *proc;
//         struct thread *thread;
//     } dest;
//
//     int sender_blocked;
//     msg_t *msg;
// };
//
// struct msg_handler {
//     ulong msg_num;
//     ulong vaddr;
// };


/*
 * Thread
 */
struct process;

enum thread_state {
    THREAD_STATE_ENTER,     // Thread just created
    THREAD_STATE_NORMAL,    // Thread running
    THREAD_STATE_WAIT,      // Thread waiting for object
    THREAD_STATE_EXIT,      // Thread waiting to be terminated
};

struct thread_memory {
    // Virtual base
    ulong block_base;
    ulong block_size;

    // Stack
    struct {
        ulong top_offset;
        ulong limit_offset;
        ulong size;
        void *top_paddr_ptr;
    } stack;

    // TLS
    struct {
        ulong start_offset;
        ulong size;
        void *start_paddr_ptr;
    } tls;

    // Msg, allocated on stack
    struct {
        ulong start_offset;
        ulong size;
        void *start_paddr_ptr;
    } msg;
};

struct thread_attri {
    int pin_cpu_mp_seq;
    ulong stack_size;
    ulong tls_size;
};

struct thread {
    list_node_t node;
    list_node_t node_proc;
    list_node_t node_sched;
    list_node_t node_wait;

    // Thread info
    ulong tid;
    int is_main;
    enum thread_state state;
    struct thread_memory memory;

    // Containing process and necessary info for switching
    ulong pid;
    int user_mode;
    void *page_table;
    ulong asid;

    // Context
    struct reg_context context;

    // Wait
    enum thread_wait_type wait_type;
    ulong wait_obj;
    u64 wait_timeout_ms;

    // CPU affinity
    int pin_cpu_id;

    // IPC
    ulong reply_msg_to_tid;

    // Lock
    spinlock_t lock;
    atomic_t ref_count;
};

#define access_thread(tid, t) \
    for (struct thread *t = acquire_thread(tid); t; release_thread(t), t = NULL) \
        for (int __term = 0; !__term; __term = 1)

#define create_and_run_thread(tid, t, p, entry, param, attri) \
    for (ulong tid = create_thread(p, entry, param, attri); tid; tid = 0) \
        for (struct thread *t = acquire_thread(tid); t; release_thread(t), run_thread(t), t = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define create_and_access_thread(tid, t, p, entry, param, attri) \
    for (ulong tid = create_thread(p, entry, param, attri); tid; tid = 0) \
        for (struct thread *t = acquire_thread(tid); t; release_thread(t), t = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define create_and_run_kernel_thread(tid, t, entry, param, attri) \
    for (struct process *p = acquire_process(get_kernel_pid()); p; release_process(p), p = NULL) \
        create_and_run_thread(tid, t, p, (ulong)(entry), (ulong)(param), attri)

#define create_and_access_kernel_thread(tid, t, entry, param, attri) \
    for (struct process *p = acquire_process(get_kernel_pid()); p; release_process(p), p = NULL) \
        create_and_access_thread(tid, t, p, (ulong)(entry), (ulong)(param), attri)

extern void init_thread();

extern struct thread *acquire_thread(ulong id);
extern void release_thread(struct thread *t);
extern int thread_exists(ulong tid);

extern ulong create_thread(struct process *p, ulong entry_point, ulong param,
                           struct thread_attri *attri);
extern void exit_thread(struct thread *t);

extern void thread_save_context(struct thread *t, struct reg_context *ctxt);

extern void set_thread_state(struct thread *t, int state);
extern void run_thread(struct thread *t);


/*
 * Process
 */
enum process_type {
    // Native types
    PROCESS_TYPE_KERNEL,
    PROCESS_TYPE_DRIVER,
    PROCESS_TYPE_SYSTEM,
    PROCESS_TYPE_USER,

    // Emulation - e.g. running Linux on top of Toddler
    PROCESS_TYPE_EMULATE,
};

enum process_state {
    PROCESS_STATE_ENTER,
    PROCESS_STATE_NORMAL,
    PROCESS_STATE_EXIT,
};

enum process_vm_block_state {
    VM_STATE_FREE_UNMAPPED,
    VM_STATE_FREE_MAPPED,
    VM_STATE_INUSE,
};

struct dynamic_block {
    list_node_t node;
    list_node_t node_free;
    list_node_t node_mapped;

    enum process_vm_block_state state;
    ulong base;
    ulong size;
};

struct process_memory {
    // Entry point
    ulong entry_point;

    // Memory layout
    struct {
        ulong start;
        ulong end;
    } program;

    struct {
        ulong start;
        ulong end;
    } heap;

    struct {
        ulong top;
        ulong bottom;

        list_t blocks;  // All blocks
        list_t free;    // Free and unmapped
        list_t mapped;  // Free but mapped
    } dynamic;
};

struct process {
    list_node_t node;

    // Process and parent ID, -1 = No parent
    ulong pid;
    ulong parent_pid;

    // Name and URL
    char *name;
    char *url;

    // Type and state
    enum process_type type;
    enum process_state state;
    int user_mode;

    // ASID
    ulong asid;

    // Page table
    void *page_table;

    // Virtual memory
    struct process_memory vm;

    // Threads
    list_t threads;
    atomic_t thread_create_count;
    atomic_t thread_exit_count;

    // Wait objects
    list_t wait_objects;

    // Scheduling
    int priority;

    // IPC
    ulong popup_msg_handler;

    // Lock
    spinlock_t lock;
    atomic_t ref_count;
};


/*
 * Dynamic VM
 */
extern void init_dynamic_vm();
extern ulong vm_alloc(struct process *p, ulong size, ulong align, ulong attri);
extern void vm_free(struct process *p, ulong base);
extern void vm_cleanup(struct process *p);
extern void vm_purge(struct process *p);


/*
 * Process
 */
#define access_process(id, proc) \
    for (struct process *proc = acquire_process(id); proc; release_process(proc), proc = NULL) \
        for (int __term = 0; !__term; __term = 1)

extern void init_process();
extern ulong get_kernel_pid();

extern struct process *acquire_process(ulong id);
extern void release_process(struct process *p);
extern int process_exists(ulong pid);

extern ulong create_process(ulong parent_id, char *name, enum process_type type);
extern int load_coreimg_elf(struct process *p, char *url, void *img);


/*
 * Start up
 */
extern void init_startup();


/*
 * Scheduling
 */
extern void init_sched();

extern void sched_put(struct thread *t);

extern void switch_to_thread(struct thread *t);
extern void sched();


/*
 * Wait
 */
#define access_wait_queue_exclusive(q) \
    for (list_t *q = acquire_wait_queue_exclusive(); q; release_wait_queue(), q = NULL) \
        for (int __term = 0; !__term; __term = 1)

extern void init_wait();
extern list_t *acquire_wait_queue_exclusive();
extern void release_wait_queue();

extern void sleep_thread(struct thread *t);

extern ulong alloc_wait_object(struct process *p, ulong user_obj_id, ulong total, int global);
extern int wait_on_object(struct process *p, struct thread *t, int wait_type, ulong obj_id, ulong timeout_ms);
extern ulong wake_on_object(struct process *p, struct thread *t, int wait_type, ulong obj_id, ulong max_wakeup_count);
extern ulong wake_on_object_exclusive(struct process *p, struct thread *t, int wait_type, ulong wait_obj, ulong max_wakeup_count);

extern void purge_wait_queue(struct process *p);


/*
 * IPC
 */
extern void init_ipc();

extern int ipc_reg_popup_handler(struct process *p, struct thread *t, ulong entry);

extern int ipc_request(struct process *p, struct thread *t, ulong dst_pid, ulong opcode, ulong flags, int *wait);
extern int ipc_respond(struct process *p, struct thread *t);
extern int ipc_receive(struct process *p, struct thread *t, ulong timeout_ms, ulong *opcode, int *wait);


// /*
//  * TLB management
//  */
// extern void init_tlb_mgmt();
// extern void trigger_tlb_shootdown(ulong asid, ulong addr, size_t size);
// extern void service_tlb_shootdown();


// /*
//  * Heap management
//  */
// extern ulong set_heap_end(struct process *p, ulong heap_end);
// extern ulong get_heap_end(struct process *p);
// extern ulong grow_heap(struct process *p, ulong amount);
// extern ulong shrink_heap(struct process *p, ulong amount);


// /*
//  * ASID management
//  */
// extern void init_asid();
// extern void asid_release();
// extern ulong asid_alloc();
// extern ulong asid_recycle();


// /*
//  * Process monitor
//  */
// extern int check_process_create_before(unsigned long parent_proc_id);
// extern int check_process_create_after(unsigned long parent_proc_id, unsigned long proc_id);
// extern int check_process_terminate_before(unsigned long proc_id);
// extern int check_process_terminate_after(unsigned long proc_id);
// extern int register_process_monitor(enum proc_monitor_type type, unsigned long proc_id, unsigned long func_num, unsigned long opcode);
// extern void init_process_monitor();


// /*
//  * KMap and mmap
//  */
// extern unsigned long kmap(struct process *p, enum kmap_region region);


#endif
