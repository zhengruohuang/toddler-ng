#ifndef __KERNEL_INCLUDE_PROC__
#define __KERNEL_INCLUDE_PROC__


#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/proc.h"
#include "kernel/include/atomic.h"
#include "kernel/include/struct.h"


/*
 * Thread
 */
struct vm_block;
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
    struct vm_block *block;

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
    int wait_type;
    ulong wait_obj;
    u64 wait_timeout_ms;

    // CPU affinity
    int pin_cpu_id;

    // IPC
    struct {
        ulong pid;
        ulong tid;
        ulong opcode;
        int need_response;
    } ipc_wait;

    struct {
        ulong pid;
        ulong tid;
    } ipc_reply_to;

    // Lock
    spinlock_t lock;
    ref_count_t ref_count;
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
extern ulong get_num_threads();

extern ulong create_thread(struct process *p, ulong entry_point, ulong param,
                           struct thread_attri *attri);
extern void exit_thread(struct thread *t);

extern void thread_save_context(struct thread *t, struct reg_context *ctxt);

extern void set_thread_state(struct thread *t, int state);
extern void run_thread(struct thread *t);


/*
 * Process
 */
enum process_state {
    PROCESS_STATE_ENTER,    // Process just created, waiting to start the first thread
    PROCESS_STATE_NORMAL,   // Process running
    PROCESS_STATE_CRASHED,  // Process crashed, waiting for system to send STOP
    PROCESS_STATE_STOPPED,  // STOP received from system, stopping all running threads
    PROCESS_STATE_ZOMBIE,   // RECYCLE received from system, freeing up all memory blocks
};

enum process_vm_type {
    VM_TYPE_AVAIL,
    VM_TYPE_GENERIC,
    VM_TYPE_THREAD,
};

enum process_vm_map_type {
    VM_MAP_TYPE_NONE,
    VM_MAP_TYPE_OWNER,
    VM_MAP_TYPE_GUEST,
};

struct vm_block {
    list_node_t node;
    list_node_t node_tlb_shootdown;

    struct process *proc;
    ulong base;
    ulong size;
    enum process_vm_type type;
    enum process_vm_map_type map_type;

    struct {
        void *tls_start_ptr;
        void *stack_top_ptr;
    } thread_map;

    volatile ulong tlb_shootdown_seq;
    volatile ulong wait_acks;
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
    } dynamic;

    list_t avail_unmapped;  // Available blocks, unmapped
    list_t sanit_unmapped;  // Blocks waiting to be freed, unmapped
    list_t sanit_mapped;    // Blocks waiting to be unmapped, mapped
    list_t reuse_mapped;    // Blocks ready for reuse, mapped
                            // Only thread blocks can be reused
    list_t inuse_mapped;    // Blocks inuse, mapped

    // Total number of blocks that don't belong to avail_unmapped
    ref_count_t num_active_blocks;

    // Total number of physical pages allocated
    ref_count_t num_palloc_pages;

    ref_count_t num_salloc_objs;
};

struct process {
    list_node_t node;

    // Process and parent ID, -1 = No parent
    ulong pid;
    ulong parent_pid;

    // Name
    char *name;

    // Type and state
    enum process_type type;
    enum process_state state;
    int user_mode;

    // ASID
    ulong asid;

    // Page table
    void *page_table;
    spinlock_t page_table_lock;

    // Virtual memory
    struct process_memory vm;

    // Threads
    list_t threads;
    ref_count_t thread_create_count;
    ref_count_t thread_exit_count;

    // Wait objects
    list_t wait_objects;

    // Scheduling
    int priority;

    // IPC
    ulong popup_msg_handler;

    // Lock
    spinlock_t lock;
    ref_count_t ref_count;
};


/*
 * Dynamic VM
 */
typedef int (*vm_block_thread_mapper_t)(struct process *p, struct thread *t, struct vm_block *b);

extern void init_vm();
extern int vm_create(struct process *p);

extern struct vm_block *vm_alloc_thread(struct process *p, struct thread *t,
                                        vm_block_thread_mapper_t mapper,
                                        vm_block_thread_mapper_t reuser);
extern struct vm_block *vm_alloc(struct process *p, ulong base, ulong size, ulong attri);

extern int vm_free_block(struct process *p, struct vm_block *b);
extern int vm_free(struct process *p, ulong base);
extern void vm_move_to_sanit_unmapped(struct process *p, struct vm_block *b);
extern void vm_move_sanit_to_avail(struct process *p);

extern ulong vm_purge(struct process *p);
extern void vm_destory(struct process *p);

extern int vm_map(struct process *p, ulong base, ulong prot);
extern ulong vm_map_coreimg(struct process *p);
extern ulong vm_map_devtree(struct process *p);
extern ulong vm_map_dev(struct process *p, ulong ppfn, ulong size, ulong prot);
extern ulong vm_map_kernel(struct process *p, ulong size, ulong prot);
extern ulong vm_map_cross(struct process *p, ulong remote_pid, ulong remote_vbase, ulong size, ulong remote_prot);


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

extern ulong get_num_processes();
extern void process_stats(ulong *count, struct proc_stat *buf, size_t buf_size);

extern ulong create_process(ulong parent_id, char *name, enum process_type type);
extern int exit_process(struct process *proc, ulong status);
extern int recycle_process(struct process *proc);

extern int load_coreimg_elf(struct process *p, void *img);


/*
 * Start up
 */
extern ulong get_system_pid();
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
extern ulong get_num_wait_threads();

extern void sleep_thread(struct thread *t);

extern ulong alloc_wait_object(struct process *p, ulong user_obj_id, ulong total, int global);
extern int wait_on_object(struct process *p, struct thread *t, int wait_type, ulong wait_obj, ulong wait_value, ulong timeout_ms);
extern ulong wake_on_object(struct process *p, struct thread *t, int wait_type, ulong wait_obj, ulong wait_value, ulong max_wakeup_count);
extern ulong wake_on_object_exclusive(struct process *p, struct thread *t, int wait_type, ulong wait_obj, ulong wait_value, ulong max_wakeup_count);

extern ulong purge_wait_queue(struct process *p);


/*
 * IPC
 */
extern void sleep_ipc_thread(struct thread *t);
extern ulong get_num_ipc_threads();

extern void init_ipc();

extern int ipc_reg_popup_handler(struct process *p, struct thread *t, ulong entry);

extern int ipc_request(struct process *p, struct thread *t, ulong dst_pid, ulong opcode, ulong flags, int *wait);
extern int ipc_respond(struct process *p, struct thread *t);
extern int ipc_receive(struct process *p, struct thread *t, ulong timeout_ms, ulong *opcode, int *wait);

extern ulong purge_ipc_queue(struct process *p);

extern int ipc_notify_system(ulong opcode, ulong pid);


/*
 * TLB management
 */
extern void init_tlb_shootdown();

extern int request_tlb_shootdown(struct process *p, struct vm_block *b);
extern void service_tlb_shootdown_requests();

extern void tlb_shootdown_stats(ulong *count, ulong *global_seq);


/*
 * Interrupt
 */
extern ulong int_handler_register(struct process *p, ulong phandle, ulong entry);
extern int int_handler_unregister(struct process *p, ulong seq);

extern int int_handler_invoke(ulong seq);
extern int int_handler_eoi(ulong seq);


/*
 * ASID
 */
extern void init_asid();
extern ulong alloc_asid();
extern void free_asid(ulong asid);


#endif
