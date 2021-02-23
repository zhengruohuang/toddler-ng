#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/mem.h"
#include "kernel/include/lib.h"
#include "kernel/include/atomic.h"
#include "kernel/include/struct.h"


#define NUM_BITS_PER_MAP    (sizeof(ulong) * 8)
#define MAX_NUM_BITS_L1     (NUM_BITS_PER_MAP)
#define MAX_NUM_BITS_L2     (NUM_BITS_PER_MAP * NUM_BITS_PER_MAP)
#define MAX_NUM_BITS_L3     (NUM_BITS_PER_MAP * NUM_BITS_PER_MAP * NUM_BITS_PER_MAP)


/*
 * Create
 */
static inline void _create_l1(id_obj_t *obj)
{
    obj->num_levels = 1;
    obj->mask1 = (0x1ul << obj->range) - 0x1ul;
}

static inline void _create_l2(id_obj_t *obj)
{
    obj->num_levels = 2;

    for (ulong l2idx = 0, count = obj->range; count; l2idx++) {
        if (count >= NUM_BITS_PER_MAP) {
            obj->mask2[l2idx] = -0x1ul;
            count -= NUM_BITS_PER_MAP;
        } else {
            obj->mask2[l2idx] = (0x1ul << count) - 0x1ul;
            count = 0;
        }

        ulong l1pos = l2idx;
        obj->mask1 |= 0x1ul << l1pos;
    }
}

static inline void _create_l3(id_obj_t *obj)
{
    obj->num_levels = 3;

    int num_pages = get_vpage_count(MAX_NUM_BITS_L3);
    obj->mask3 = palloc_ptr(num_pages);

    for (ulong l3idx = 0, count = obj->range; count; l3idx++) {
        if (count >= NUM_BITS_PER_MAP) {
            obj->mask3[l3idx] = -0x1ul;
            count -= NUM_BITS_PER_MAP;
        } else {
            obj->mask3[l3idx] = (0x1ul << count) - 0x1ul;
            count = 0;
        }

        ulong l2idx = l3idx / NUM_BITS_PER_MAP;
        ulong l2pos = l3idx % NUM_BITS_PER_MAP;
        obj->mask2[l2idx] |= 0x1ul << l2pos;

        ulong l1pos = l2idx;
        obj->mask1 |= 0x1ul << l1pos;
    }
}

void id_alloc_create(id_obj_t *obj, ulong first, ulong last)
{
    memzero(obj, sizeof(id_obj_t));
    obj->base = first;
    obj->range = last - first + 1;
    obj->alloc_count = 0;

    if (obj->range <= MAX_NUM_BITS_L1) {
        _create_l1(obj);
    } else if (obj->range <= MAX_NUM_BITS_L2) {
        _create_l2(obj);
    } else if (obj->range <= MAX_NUM_BITS_L3) {
        _create_l3(obj);
    } else {
        obj->range = MAX_NUM_BITS_L3;
        _create_l3(obj);
        panic("Range exceeding max limit: %lu\n", MAX_NUM_BITS_L3);
    }

    obj->bitmaps[1] = &obj->mask1;
    obj->bitmaps[2] = obj->mask2;
    obj->bitmaps[3] = obj->mask3;

    // Lists and Dicts will be protected by ID obj's lock
    spinlock_init(&obj->lock);
}


/*
 * Alloc
 */
long id_alloc(id_obj_t *obj)
{
    long id = -1;

    spinlock_exclusive_int(&obj->lock) {
        if (obj->alloc_count == obj->range) {
            break;
        }

        ulong bitmap_idx = 0;
        ulong bitmap_pos = 0;
        ulong *bitmap = NULL;

        for (int level = 1; level <= obj->num_levels; level++) {
            bitmap = &obj->bitmaps[level][bitmap_idx];

            bitmap_pos = ctz(*bitmap);
            bitmap_idx *= NUM_BITS_PER_MAP;
            bitmap_idx += bitmap_pos;
        }

        id = obj->base + bitmap_idx;

        *bitmap &= ~(0x1ul << bitmap_pos);
        if (!*bitmap) {
            ulong clear_idx = bitmap_idx / NUM_BITS_PER_MAP;
            for (int level = obj->num_levels - 1; level; level--) {
                bitmap_idx = clear_idx / NUM_BITS_PER_MAP;
                bitmap_pos = clear_idx % NUM_BITS_PER_MAP;

                bitmap = &obj->bitmaps[level][bitmap_idx];
                *bitmap &= ~(0x1ul << bitmap_pos);
                if (*bitmap) {
                    break;
                }

                clear_idx = bitmap_idx;
            }
        }
    }

    return id;
}


/*
 * Free
 */
void id_free(id_obj_t *obj, ulong id)
{
    if (!obj) {
        return;
    }

    ulong idx = id - obj->base;
    panic_if(idx >= obj->range, "ID to free exceeding range!\n");

    spinlock_exclusive_int(&obj->lock) {
        for (int level = obj->num_levels; level; level--) {
            ulong bitmap_idx = idx / NUM_BITS_PER_MAP;
            ulong bitmap_pos = idx % NUM_BITS_PER_MAP;

            ulong *bitmap = &obj->bitmaps[level][bitmap_idx];
            assert(bitmap);

            ulong mask = 0x1ul << bitmap_pos;
            if (level == obj->num_levels) {
                panic_if(*bitmap & mask, "Inconsistent ID alloc!\n");
            }
            *bitmap |= mask;

            idx = bitmap_idx;
        }

        obj->alloc_count--;
    }
}
