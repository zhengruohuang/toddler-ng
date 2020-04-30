#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifndef AVOID_LIBC_CONFLICT
#define AVOID_LIBC_CONFLICT
#endif

#include "common/include/abi.h"
#include "common/include/compiler.h"
#include "common/include/coreimg.h"


#define BUFFER_SIZE     512


static FILE *image;
static u8 *buffer;
static struct coreimg_fat *fat;


static int is_big_endian()
{
    unsigned int val = 0xbeefbeef;
    unsigned char *first_byte = (void *)&val;

    return *first_byte == 0xbe;
}

static u32 swap_endian32(u32 val)
{
    u32 rr = val & 0xff;
    u32 rl = (val >> 8) & 0xff;
    u32 lr = (val >> 16) & 0xff;
    u32 ll = (val >> 24) & 0xff;

    u32 swap = (rr << 24) | (rl << 16) | (lr << 8) | ll;
    return swap;
}

static u32 swap_to_target_endian(u32 val)
{
    return is_big_endian() != ARCH_BIG_ENDIAN ? swap_endian32(val) : val;
}

static void copy_to_image(const void *buf, u32 offset, unsigned long size)
{
    fseek(image, offset, SEEK_SET);
    fwrite(buf, size, 1, image);
}

static void extract_file_name(char *extracted_name, char *full_name)
{
    int i;

    int name_start = 0;
    int name_end = strlen(full_name);

    for (i = name_end; i >= 0; i--) {
        if (full_name[i] == '\\' || full_name[i] == '/') {
            name_start = i + 1;
            break;
        }
    }

    int name_length = name_end - name_start;
    if (name_length > 19) {
        name_length = 19;
    }

    for (i = 0; i < 20; i++) {
        extracted_name[i] = '\0';
    }

    for (i = 0; i < name_length; i++) {
        extracted_name[i] = full_name[name_start + i];
    }
}

static int gen_image(int argc, char *argv[])
{
    // Initialize buffer and the image
    buffer = (u8 *)malloc(BUFFER_SIZE);
    image = fopen(argv[1], "wb");
    if (0 == image) {
        printf("\tUnable to create output file!\n");
        return -1;
    }

    u32 image_size = 0;
    u32 cur_offset = 0;

    char *cur_file_name;
    FILE *cur_file;
    u32 cur_file_size = 0;
    u32 read_count;

    u32 i;

    // Calculate FAT count and file count
    u32 file_count = argc - 2;
    u32 fat_entry_count = file_count + 1;
    u32 fat_size = fat_entry_count * sizeof(struct coreimg_record);
    u32 file_start_offset = fat_size;
    cur_offset = file_start_offset;
    image_size += fat_size;
    fat = (struct coreimg_fat *)malloc(fat_size);

    printf("\tFile Count: %d\n", file_count);
    printf("\tFAT Size: %d\n", fat_size);

    // Process files
    for (i = 0; i < file_count; i++) {
        cur_file_name = argv[2 + i];
        printf("\tProcessing: %s ... ", cur_file_name);

        if (!cur_file_name) {
            printf("failed\n");
            return -1;
        }

        // Open the file
        cur_file_size = 0;
        cur_file = fopen(cur_file_name, "rb");
        if (!cur_file) {
            printf("ignored\n");
            continue;
            //return -1;
        }

        // Set file info in FAT
        fat->records[i].file_type = 1;
        fat->records[i].load_type = 1;
        fat->records[i].compressed = 0;

        // Set file start position in FAT
        fat->records[i].start_offset = cur_offset;

        // Load current file to memory, and write to the image at the same time
        do {
            // Read the file
            read_count = fread(buffer, 1, BUFFER_SIZE, cur_file);

            // Copy the file to the image
            copy_to_image(buffer, cur_offset, read_count);

            // Update size and offset */
            cur_offset += read_count;
            cur_file_size += read_count;
        } while (!feof(cur_file));

        // Done processing current file
        fclose(cur_file);

        // Refine file size: align to 8byte
        if (cur_file_size % 8) {
            cur_file_size = cur_file_size >> 3;
            cur_file_size++;
            cur_file_size = cur_file_size << 3;
        }
        fat->records[i].length = cur_file_size;
        image_size += cur_file_size;

        // Extract file name
        extract_file_name(fat->records[i].file_name, cur_file_name);

        // Update offset
        cur_offset = fat->records[i].start_offset + cur_file_size;

        // Fix endian
        fat->records[i].start_offset = swap_to_target_endian(fat->records[i].start_offset);
        fat->records[i].length = swap_to_target_endian(fat->records[i].length);

        printf("done\n");
    }

    // Fix FAT header
    fat->header.magic = swap_to_target_endian(COREIMG_MAGIC);
    fat->header.file_count = swap_to_target_endian(file_count);
    fat->header.image_size = swap_to_target_endian(image_size);
    fat->header.big_endian = ARCH_BIG_ENDIAN;

    // Write FAT to image
    printf("\tProcessing FAT ... ");
    copy_to_image(fat, 0, fat_size);
    printf("done\n");

    // Done processing the image
    fclose(image);

    return 0;
}

static void usage()
{
    printf("Usage:\n");
    printf("\tcoreimg <name> [file1, file2, ..., file n]\n");
}

int main(int argc, char *argv[])
{
    printf("Toddler core image generator 0.5.1\n");

    // Check arguments
    if (argc < 2) {
        usage();
    } else {
        int result = gen_image(argc, argv);

        if (result) {
            printf("\tUnable to generate core image!\n");
        } else {
            printf("\tCore image generated successfully!\n");
        }
    }

    return 0;
}
