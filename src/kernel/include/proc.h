#ifndef __KERNEL_INCLUDE_PROC__
#define __KERNEL_INCLUDE_PROC__


#include "common/include/inttypes.h"
#include "common/include/context.h"
// #include "common/include/proc.h"
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


// /*
//  * Scheduling
//  */
// enum sched_state {
//     sched_enter,
//     sched_ready,
//     sched_run,
//     sched_idle,
//     sched_stall,
//     sched_exit,
// };
//
// struct sched {
//     // Sched list
//     struct sched *prev;
//     struct sched *next;
//
//     // Sched control info
//     ulong sched_id;
//     enum sched_state state;
//
//     // Priority
//     int base_priority;
//     int priority;
//     int is_idle;
//
//     // Containing proc and thread
//     ulong proc_id;
//     struct process *proc;
//     ulong thread_id;
//     struct thread *thread;
//
//     // Affinity
//     int pin_cpu_id;
// };
//
// struct sched_list {
//     ulong count;
//     struct sched *head;
//     struct sched *tail;
//
//     spinlock_t lock;
// };


/*
 * Thread
 */
struct process;

enum thread_state {
    THREAD_STATE_ENTER, // Thread just created
    THREAD_STATE_SCHED, // Thread is being sched
    THREAD_STATE_NORMAL,    // Thread running
    THREAD_STATE_STALL, // Thread waiting for syscall or IO reponse
    THREAD_STATE_WAIT,
    THREAD_STATE_EXIT,  // Thread waiting to be terminated
    THREAD_STATE_CLEAN, // Thread terminated, waiting to be claned
};

struct thread_memory {
    // Virtual base
    ulong block_base;
    ulong block_size;

    // Stack
    ulong stack_top_offset;
    ulong stack_limit_offset;
    ulong stack_size;
    ulong stack_top_paddr;

    // TCB
    ulong tcb_start_offset;
    ulong tcb_size;
    ulong tcb_start_paddr;

    // TLS
    ulong tls_start_offset;
    ulong tls_size;
    ulong tls_start_paddr;

    // Msg recv
    ulong msg_recv_offset;
    ulong msg_recv_size;
    ulong msg_recv_paddr;

    // Msg send
    ulong msg_send_offset;
    ulong msg_send_size;
    ulong msg_send_paddr;
};

struct thread {
    // Thread info
    ulong tid;
    enum thread_state state;
    struct thread_memory memory;

    // Containing process and necessary info for switching
    ulong pid;
    int user_mode;
    void *page_table;
    ulong asid;

    // Context
    struct reg_context context;

    // CPU affinity
    int pin_cpu_id;

    // Scheduling
    //ulong sched_id;
    //struct sched *sched;

    // IPC
    //struct msg_node *cur_msg;

    // Lock
    spinlock_t lock;
    atomic_t ref_count;
};

#define thread_access_exclusive(tid, t) \
    for (struct thread *t = acquire_thread(tid); t; release_thread(t), t = NULL) \
        for (int __term = 0; !__term; __term = 1)

#define create_thread_and_run(tid, t, p, entry, param, pin, stack, tls) \
    for (ulong tid = create_thread(p, entry, param, pin, stack, tls); tid; tid = 0) \
        for (struct thread *t = acquire_thread(tid); t; release_thread(t), t = NULL) \
            for (int __term = 0; !__term; run_thread(t)) \
                for (; !__term; __term = 1)

#define create_thread_and_access_exclusive(tid, t, p, entry, param, pin, stack, tls) \
    for (ulong tid = create_thread(p, entry, param, pin, stack, tls); tid; tid = 0) \
        for (struct thread *t = acquire_thread(tid); t; release_thread(t), t = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define create_krun(tid, t, entry, param, pin) \
    for (struct process *p = acquire_process(get_kernel_pid()); p; release_process(p), p = NULL) \
        create_thread_and_run(tid, t, p, (ulong)(entry), (ulong)(param), pin, 0, 0)

#define create_kthread(tid, t, entry, param, pin) \
    for (struct process *p = acquire_process(get_kernel_pid()); p; release_process(p), p = NULL) \
        create_thread_and_access_exclusive(tid, t, p, (ulong)(entry), (ulong)(param), pin, 0, 0)

extern void init_thread();

extern struct thread *acquire_thread(ulong id);
extern void release_thread(struct thread *t);

extern ulong create_thread(struct process *p, ulong entry_point, ulong param,
                                    int pin_cpu_id, ulong stack_size, ulong tls_size);
extern void run_thread(struct thread *t);

extern void thread_save_context(struct thread *t, struct reg_context *ctxt);


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

struct process_memory {
    // Entry point
    ulong entry_point;

    // Memory layout
    ulong program_start;
    ulong program_end;

    ulong dynamic_top;
    ulong dynamic_bottom;

    ulong heap_start;
    ulong heap_end;
};

struct dynamic_block {
    struct dynamic_block *next;

    ulong base;
    ulong size;
};

struct dynamic_block_list {
    struct dynamic_block *head;
    int count;
};

struct dynamic_area {
    ulong cur_top;

    struct dynamic_block_list in_use;
    struct dynamic_block_list free;
};

struct process {
    // Process and parent ID, -1 = No parent
    ulong pid;
    ulong parent_pid;

    // ASID
    ulong asid;

    // Name and URL
    char *name;
    char *url;

    // Type and state
    enum process_type type;
    enum process_state state;
    int user_mode;

    // Page table
    void *page_table;

    // Virtual memory
    struct process_memory memory;

    // Dynamic area map
    struct dynamic_area dynamic;

    // Thread
    slist_t threads;

    // Scheduling
    uint priority;

    // IPC
    ulong mailbox_id;
    //list_t msgs;
    //hashtable_t msg_handlers;

    // Lock
    spinlock_t lock;
    atomic_t ref_count;
};

struct process_list {
    ulong count;
    struct process *next;

    spinlock_t lock;
};


// /*
//  * Dynamic area
//  */
// extern void init_dalloc();
// extern void create_dalloc(struct process *p);
// extern ulong dalloc(struct process *p, ulong size);
// extern void dfree(struct process *p, ulong base);


/*
 * Process
 */
#define process_access_exclusive(id, proc) \
    for (struct process *proc = acquire_process(id); proc; release_process(proc), proc = NULL) \
        for (int __term = 0; !__term; __term = 1)

extern void init_process();
extern ulong get_kernel_pid();

extern struct process *acquire_process(ulong id);
extern void release_process(struct process *p);

extern ulong create_process(ulong parent_id, char *name, enum process_type type);
extern int load_coreimg_elf(struct process *p, char *url, void *img);


/*
 * Start up
 */
extern void init_startup();


// /*
//  * Thread
//  */
// extern struct thread *gen_thread_by_thread_id(ulong thread_id);
// extern void init_thread();
//
// extern void create_thread_lists(struct process *p);
// extern struct thread *create_thread(
//     struct process *p, ulong entry_point, ulong param,
//     int pin_cpu_id,
//     ulong stack_size, ulong tls_size
// );
//
// extern void set_thread_arg(struct thread *t, ulong arg);
// extern void change_thread_control(struct thread *t, ulong entry_point, ulong param);
//
// extern void destroy_absent_threads(struct process *p);
//
// extern void run_thread(struct thread *t);
// extern void idle_thread(struct thread *t);
// extern int wait_thread(struct thread *t);
// extern void terminate_thread_self(struct thread *t);
// extern void terminate_thread(struct thread *t);
// extern void clean_thread(struct thread *t);
//
// extern asmlinkage void kernel_idle_thread(ulong param);
// extern asmlinkage void kernel_demo_thread(ulong param);
// extern asmlinkage void kernel_tclean_thread(ulong param);


/*
 * Scheduling
 */
extern void init_sched();

extern void sched_put(struct thread *t);

extern void switch_to_thread(struct thread *t);
extern void sched();

// extern struct sched *get_sched(ulong sched_id);
//
// extern struct sched *enter_sched(struct thread *t);
// extern void ready_sched(struct sched *s);
// extern void idle_sched(struct sched *s);
// extern void wait_sched(struct sched *s);
// extern void exit_sched(struct sched *s);
// extern void exit_sched(struct sched *s);
// extern void clean_sched(struct sched *s);
//
// extern int desched(ulong sched_id, struct context *context);
// extern void resched(ulong sched_id);
// extern void sched();


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
