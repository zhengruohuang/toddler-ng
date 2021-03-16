#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/hal.h"
#include "hal/include/dev.h"


/*
 * Num CPUs
 */
static int num_cpus = 1;

int get_num_cpus()
{
    return num_cpus;
}

int is_single_cpu()
{
    return num_cpus == 1;
}


/*
 * MP seq to ID
 */
static ulong *mp_seq_to_id = NULL;

void set_num_cpus(int n)
{
    panic_if(n <= 0, "Invalid num of CPUs: %d\n", n);
    num_cpus = n;

    mp_seq_to_id = mempool_alloc(sizeof(ulong) * num_cpus);
    memzero(mp_seq_to_id, sizeof(ulong) * num_cpus);
}

void set_mp_trans(int mp_seq, ulong mp_id)
{
    panic_if(mp_seq >= num_cpus,
             "Invalid MP seq %ld exceeding num CPUs\n", mp_seq);
    mp_seq_to_id[mp_seq] = mp_id;
}

void final_mp_trans()
{
    ulong boot_mp_id = arch_get_cur_mp_id();
    panic_if(mp_seq_to_id[0] != boot_mp_id, "Boot CPU must have MP seq 0!\n");

//     // FIXME: deprecated
//     if (mp_seq_to_id[0] == boot_mp_id) {
//         return;
//     }
//
//     // Make sure bootstrap CPU has mp_seq == 0
//     for (int mp_seq = 1; mp_seq < num_cpus; mp_seq++) {
//         if (mp_seq_to_id[mp_seq] == boot_mp_id) {
//             mp_seq_to_id[mp_seq] = mp_seq_to_id[0];
//             mp_seq_to_id[0] = boot_mp_id;
//             return;
//         }
//     }
//
//     panic("Inconsistent mp seq to ID translation table!\n");
}

ulong get_mp_id_by_seq(int mp_seq)
{
    panic_if(mp_seq >= num_cpus,
             "Invalid MP seq %ld exceeding num CPUs\n", mp_seq);
    return mp_seq_to_id[mp_seq];
}



// /*
//  * FIXME: deprecated
//  * Arch Multiprocessor ID <-> Seq
//  */
// #define MAX_ENTRIES_PER_FIELD   128
// #define MAX_TRANS_TABLE_SIZE    2048
//
// struct mp_id_trans_field {
//     int start;
//     int bits;
//     ulong mask;
//     ulong min_val, max_val;
//     int valid;
//     ulong num_entries;
// };
//
// struct mp_id_trans_entry {
//     union {
//         struct mp_id_trans_entry *next;
//         int seq;
//     };
// };
//
// static int num_trans_fields = 0;
// static struct mp_id_trans_field *trans_fields = NULL;
//
// static ulong *mp_seq_to_id = NULL;
// static struct mp_id_trans_entry *mp_id_to_seq = NULL;
//
// // ... = Pairs of (start, bits), higher bits go first
// static void setup_trans_field(int i, int start, int bits)
// {
//     struct mp_id_trans_field *field = &trans_fields[i];
//     field->start = start;
//     field->bits = bits;
//     field->mask = (0x1ul << bits) - 0x1ul;
//     field->min_val = -0x1ul;
//     field->valid = 1;
//
//     //kprintf("setup field: %d, start: %d, bits: %d, mask: %lx\n",
//     //        i, start, bits, field->mask);
// }
//
// void setup_mp_id_trans(int cpus, int fields, ...)
// {
//     panic_if(cpus <= 0, "Inivalid number of CPUs: %ld\n", cpus);
//     panic_if(fields < 0, "Inivalid number of trans fields: %d\n", fields);
//
//     num_cpus = cpus;
//     mp_seq_to_id = mempool_alloc(sizeof(ulong) * cpus);
//     memzero(mp_seq_to_id, sizeof(ulong) * cpus);
//
//     num_trans_fields = fields;
//     trans_fields = mempool_alloc(sizeof(struct mp_id_trans_field) * fields);
//     memzero(trans_fields, sizeof(struct mp_id_trans_field) * fields);
//
//     if (fields) {
//         va_list valist;
//         va_start(valist, fields);
//
//         for (int i = 0; i < fields; i++) {
//             int start = va_arg(valist, int);
//             int bits = va_arg(valist, int);
//             setup_trans_field(i, start, bits);
//         }
//
//         va_end(valist);
//     } else {
//         setup_trans_field(0, 0, sizeof(ulong) * 8);
//     }
// }
//
// void add_mp_id_trans(ulong mp_id)
// {
//     static int cur_adding_seq = 0;
//     mp_seq_to_id[cur_adding_seq++] = mp_id;
//
//     for (int i = 0; i < num_trans_fields; i++) {
//         struct mp_id_trans_field *field = &trans_fields[i];
//         ulong val = (mp_id >> field->start) & field->mask;
//         if (val > field->max_val) field->max_val = val;
//         if (val < field->min_val) field->min_val = val;
//     }
// }
//
// static void validate_order()
// {
//     for (int i = 1; i < num_trans_fields; i++) {
//         struct mp_id_trans_field *field = &trans_fields[i];
//         struct mp_id_trans_field *higher_field = &trans_fields[i - 1];
//
//         panic_if(field->start + field->bits < higher_field->start,
//                  "Invalid field order, higher bits must go first!");
//     }
// }
//
// static void calc_actual_mp_id_trans_field_size()
// {
//     for (int i = 0; i < num_trans_fields; i++) {
//         struct mp_id_trans_field *field = &trans_fields[i];
//
//         field->num_entries = field->max_val - field->min_val + 0x1ul;
//         panic_if(field->num_entries > MAX_ENTRIES_PER_FIELD,
//                  "Too many entries per field: %ld\n", field->num_entries);
//
//         if (field->num_entries == 1) {
//             field->valid = 0;
//         }
//     }
// }
//
// static void remove_empty_mp_id_trans_fields()
// {
//     int copy_idx = 0;
//
//     for (int scan_idx = 0; scan_idx < num_trans_fields; scan_idx++) {
//         struct mp_id_trans_field *scan_field = &trans_fields[scan_idx];
//         while (!scan_field->valid) {
//             scan_idx++;
//             if (scan_idx >= num_trans_fields) {
//                 break;
//             }
//             scan_field = &trans_fields[scan_idx];
//         }
//
//         if (scan_idx >= num_trans_fields) {
//             break;
//         }
//
//         if (copy_idx != scan_idx) {
//             memcpy(&trans_fields[copy_idx], &trans_fields[scan_idx],
//                    sizeof(struct mp_id_trans_field));
//         }
//         copy_idx++;
//     }
//
//     num_trans_fields = copy_idx;
//     kprintf("final num_trans_fields: %d\n", num_trans_fields);
// }
//
// static void construct_mp_id_trans_table()
// {
//     if (!num_trans_fields) {
//         return;
//     }
//
//     ulong total_num_entries = 1;
//     for (int i = 0; i < num_trans_fields; i++) {
//         struct mp_id_trans_field *field = &trans_fields[i];
//         total_num_entries *= field->num_entries;
//     }
//
//     ulong table_size = total_num_entries * sizeof(struct mp_id_trans_entry);
//     panic_if(table_size > MAX_TRANS_TABLE_SIZE, "MP ID table too big!\n");
//
//     int buf_idx = 0;
//     mp_id_to_seq = mempool_alloc(table_size);
//     buf_idx += trans_fields[0].num_entries;
//
//     for (ulong seq = 0; seq < num_cpus; seq++) {
//         ulong mp_id = mp_seq_to_id[seq];
//         struct mp_id_trans_entry *tab = mp_id_to_seq;
//
//         for (int i = 0; i < num_trans_fields; i++) {
//             struct mp_id_trans_field *field = &trans_fields[i];
//             ulong idx = ((mp_id >> field->start) & field->mask) - field->min_val;
//
//             if (i == num_trans_fields - 1) {
//                 tab[idx].seq = seq;
//             } else if (tab[idx].next) {
//                 tab = tab[idx].next;
//             } else {
//                 tab = mp_id_to_seq + buf_idx;
//                 buf_idx += trans_fields[i + 1].num_entries;
//                 tab[idx].next = tab;
//             }
//         }
//     }
// }
//
// void final_mp_id_trans()
// {
//     validate_order();
//     calc_actual_mp_id_trans_field_size();
//     remove_empty_mp_id_trans_fields();
//     construct_mp_id_trans_table();
// }
//
// ulong get_mp_id_by_seq(int mp_seq)
// {
//     panic_if(mp_seq >= num_cpus, "Invalid seq: %ld\n", mp_seq);
//     return mp_seq_to_id[mp_seq];
// }
//
// int get_mp_seq_by_id(ulong mp_id)
// {
//     struct mp_id_trans_entry *tab = mp_id_to_seq;
//
//     for (int i = 0; i < num_trans_fields; i++) {
//         struct mp_id_trans_field *field = &trans_fields[i];
//         ulong idx = ((mp_id >> field->start) & field->mask) - field->min_val;
//
//         if (i == num_trans_fields - 1) {
//             return tab[idx].seq;
//         } else {
//             tab = tab[idx].next;
//         }
//     }
//
//     // No translation needed, which means it's a single-CPU system
//     panic_if(num_cpus != 1, "Inconsistent num_cpus with num_trans_fields!");
//     return 0;
// }
//
// int get_cur_mp_seq()
// {
//     ulong mp_id = arch_get_cur_mp_id();
//     return get_mp_seq_by_id(mp_id);
// }


/*
 * Init
 */
void init_topo()
{
    kprintf("Detecting processor topology\n");

    num_cpus = 1;
    drv_func_detect_topology();
    kprintf("Number of logical CPUs: %d\n", num_cpus);
}
