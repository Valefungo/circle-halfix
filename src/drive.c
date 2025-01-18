// A set of drivers that regulates access to external files.
// All disk image reads/writes go through this single function

#include "drive.h"
#include "platform.h"
#include "state.h"
#include "util.h"
#include "noSDL.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void (*global_cb)(void*, int);
static void* global_cb_arg1;

static int transfer_in_progress = 0;

void drive_cancel_transfers(void)
{
    transfer_in_progress = 0;
}

#define BLOCK_SHIFT 18
#define BLOCK_SIZE (256 * 1024)
#define BLOCK_MASK (BLOCK_SIZE - 1)

// ============================================================================
// Basic drive wrapper functions
// ============================================================================

int drive_read(struct drive_info* info, void* a, void* b, uint32_t c, drv_offset_t d, drive_cb e)
{
    return info->read(info->data, a, b, c, d, e);
}
int drive_prefetch(struct drive_info* info, void* a, uint32_t b, drv_offset_t c, drive_cb d)
{
    return info->prefetch(info->data, a, b, c, d);
}
int drive_write(struct drive_info* info, void* a, void* b, uint32_t c, drv_offset_t d, drive_cb e)
{
    return info->write(info->data, a, b, c, d, e);
}

// ============================================================================
// Struct definitions for block drivers.
// ============================================================================

struct block_info {
    uint32_t pathindex;
    uint32_t modified;
    uint8_t* data; // Note: on the Emscripten version, it will not be valid pointer. Instead, it will be an ID pointing to the this.blocks[] cache.
};

struct drive_internal_info {
    // IDE callback
    void (*callback)(void*, int);
    void* ide_callback_arg1;
    // Argument 2 is supplied by the status parameter of drive_handler_callback

    // Destination
    void* argument_buffer;
    drv_offset_t argument_position, argument_length;

    drv_offset_t size;
    uint32_t block_count, block_size, path_count; // Size of image, number of blocks, size of block, and paths
    char** paths; // The number of paths present in our disk image.
    struct block_info* blocks;
};

// Contained in each file directory as info.dat
struct drive_info_file {
    uint32_t size;
    uint32_t block_size;
};

// ============================================================================
// Path utilities
// ============================================================================

void drive_check_complete(void)
{
    if (transfer_in_progress) {
        global_cb(global_cb_arg1, 0);
        transfer_in_progress = 0;
    }
}

int drive_async_event_in_progress(void)
{
    return transfer_in_progress;
}


void drive_state(struct drive_info* info, char* filename)
{
    info->state(info->data, filename);
}


// RAMDISK driver
struct ramdisk_driver {
    unsigned char *ramdisk;

    // Size of image file (in bytes) and size of each block (in bytes)
    drv_offset_t image_size, block_size;

    // Set if disk is write protected
    int write_protected;

    // Should we modify the backing file?
    int raw_file_access;

    // Size of block array, in terms of uint8_t*
    uint32_t block_array_size;

    // Table of blocks
    uint8_t** blocks;
};

// Simple driver
struct simple_driver {
    int fd; // File descriptor of image file

    // Size of image file (in bytes) and size of each block (in bytes)
    drv_offset_t image_size, block_size;

    // Size of block array, in terms of uint8_t*
    uint32_t block_array_size;

    // Set if disk is write protected
    int write_protected;

    // Should we modify the backing file?
    int raw_file_access;

    // Table of blocks
    uint8_t** blocks;
};

static void drive_simple_state(void* this, char* path)
{
    UNUSED(this);
    UNUSED(path);
}

static int drive_simple_prefetch(void* this_ptr, void* cb_ptr, uint32_t length, drv_offset_t position, drive_cb cb)
{
    // Since we're always sync, no need to prefetch
    UNUSED(this_ptr);
    UNUSED(cb_ptr);
    UNUSED(length);
    UNUSED(position);
    UNUSED(cb);
    return DRIVE_RESULT_SYNC;
}

static inline int drive_simple_has_cache(struct simple_driver* info, drv_offset_t offset)
{
    return info->blocks[offset / info->block_size] != NULL;
}

// Reads 512 bytes of data from the cache, if possible.
static int drive_simple_fetch_cache(struct simple_driver* info, void* buffer, drv_offset_t offset)
{
    if (offset & 511)
        DRIVE_FATAL("Offset not aligned to 512 byte boundary");
    uint32_t blockid = offset / info->block_size;

    // Check if block cache is open
    if (info->blocks[blockid]) {
        // Get the offset inside the block, get the physical position of the block, and copy 512 bytes into the destination buffer
        uint32_t block_offset = offset % info->block_size;
        void* ptr = info->blocks[blockid] + block_offset;
        memcpy(buffer, ptr, 512); // Copy all the bytes from offset to the end of the file
        return 1;
    }

    return 0;
}

static int drive_simple_add_cache(struct simple_driver* info, drv_offset_t offset)
{
    void* dest = info->blocks[offset / info->block_size] = malloc(info->block_size);
    lseek(info->fd, offset & (drv_offset_t) ~(info->block_size - 1), SEEK_SET); // Seek to the beginning of the current block
    if ((uint32_t)read(info->fd, dest, info->block_size) != info->block_size)
        DRIVE_FATAL("Unable to read %d bytes from image file\n", (int)info->block_size);
    return 0;
}

static inline int drive_simple_write_cache(struct simple_driver* info, void* buffer, drv_offset_t offset)
{
    uint32_t block_offset = offset % info->block_size;
    void* ptr = info->blocks[offset / info->block_size] + block_offset;
    memcpy(ptr, buffer, 512); // Copy all the bytes from offset to the end of the file
    return 0;
}

static int drive_simple_write(void* this, void* cb_ptr, void* buffer, uint32_t size, drv_offset_t offset, drive_cb cb)
{
    UNUSED(this);
    UNUSED(cb);
    UNUSED(cb_ptr);

    if ((size | offset) & 511)
        DRIVE_FATAL("Length/offset must be multiple of 512 bytes\n");

    struct simple_driver* info = this;

    drv_offset_t end = size + offset;
    while (offset != end) {
        if (!info->raw_file_access) {
            if (!drive_simple_has_cache(info, offset))
                drive_simple_add_cache(info, offset);
            drive_simple_write_cache(info, buffer, offset);
        } else {
            UNUSED(drive_simple_add_cache);
            lseek(info->fd, offset, SEEK_SET);
            if (write(info->fd, buffer, 512) != 512)
                DRIVE_FATAL("Unable to write 512 bytes to image file\n");
        }
        buffer += 512;
        offset += 512;
    }
    return DRIVE_RESULT_SYNC;
}

static int drive_simple_read(void* this, void* cb_ptr, void* buffer, uint32_t size, drv_offset_t offset, drive_cb cb)
{
    UNUSED(this);
    UNUSED(cb);
    UNUSED(cb_ptr);

    if ((size | offset) & 511)
        DRIVE_FATAL("Length/offset must be multiple of 512 bytes\n");

    struct simple_driver* info = this;

    drv_offset_t end = size + offset;
    while (offset != end) {
        if (!drive_simple_fetch_cache(info, buffer, offset)) {
            lseek(info->fd, offset, SEEK_SET);
            if (read(info->fd, buffer, 512) != 512)
                DRIVE_FATAL("Unable to read 512 bytes from image file\n");
        }
        buffer += 512;
        offset += 512;
    }
    return DRIVE_RESULT_SYNC;
}

int drive_simple_init(struct drive_info* info, char* filename)
{
    char deb[200] = "";

    uint64_t fsize = noSDL_fileGetSize(filename);
    if (fsize == 0)
        return -1;

    sprintf(deb, "FILE DISK: Disk %s is %lu", filename, fsize);
    noSDL_Kernel_Log(deb);

    int fd;
    if (!info->modify_backing_file)
        fd = open(filename, O_RDONLY | O_BINARY);
    else
        fd = open(filename, O_RDWR | O_BINARY);
    if (fd < 0)
        return -1;

    struct simple_driver* sync_info = malloc(sizeof(struct simple_driver));
    info->data = sync_info;
    sync_info->fd = fd;
    sync_info->image_size = fsize;
    sync_info->block_size = BLOCK_SIZE;
    sync_info->block_array_size = (fsize + sync_info->block_size - 1) / sync_info->block_size;
    sync_info->blocks = calloc(sizeof(uint8_t*), sync_info->block_array_size);

    sync_info->raw_file_access = info->modify_backing_file;

    info->read = drive_simple_read;
    info->state = drive_simple_state;
    info->write = drive_simple_write;
    info->prefetch = drive_simple_prefetch;

    // Now determine drive geometry
    info->sectors = fsize / 512;
    info->sectors_per_cylinder = 63;
    info->heads = 16;
    info->cylinders_per_head = info->sectors / (info->sectors_per_cylinder * info->heads);

    sprintf(deb, "FILE DISK: OK.");
    noSDL_Kernel_Log(deb);

    return 0;
}

void drive_destroy_simple(struct drive_info* info)
{
    struct simple_driver* simple_info = info->data;
    for (unsigned int i = 0; i < simple_info->block_array_size; i++)
        free(simple_info->blocks[i]);
    free(simple_info->blocks);
    free(simple_info);
}



static void drive_ramdisk_state(void* this, char* path)
{
    UNUSED(this);
    UNUSED(path);
}

static int drive_ramdisk_prefetch(void* this_ptr, void* cb_ptr, uint32_t length, drv_offset_t position, drive_cb cb)
{
    // Since we're always sync, no need to prefetch
    UNUSED(this_ptr);
    UNUSED(cb_ptr);
    UNUSED(length);
    UNUSED(position);
    UNUSED(cb);
    return DRIVE_RESULT_SYNC;
}

static int drive_ramdisk_write(void* this, void* cb_ptr, void* buffer, uint32_t size, drv_offset_t offset, drive_cb cb)
{
    UNUSED(cb);
    UNUSED(cb_ptr);

    if ((size | offset) & 511)
        DRIVE_FATAL("Length/offset must be multiple of 512 bytes\n");

    struct ramdisk_driver* info = this;

    memcpy(info->ramdisk+offset, buffer, size);

    return DRIVE_RESULT_SYNC;
}

static int drive_ramdisk_read(void* this, void* cb_ptr, void* buffer, uint32_t size, drv_offset_t offset, drive_cb cb)
{
    UNUSED(cb);
    UNUSED(cb_ptr);

    if ((size | offset) & 511)
        DRIVE_FATAL("Length/offset must be multiple of 512 bytes\n");

    struct ramdisk_driver* info = this;

    memcpy(buffer, info->ramdisk + offset, size);

    return DRIVE_RESULT_SYNC;
}

int drive_ramdisk_init(struct drive_info* info, char* filename)
{
    char deb[200] = "";

    sprintf(deb, "RAMDISK: reading size of %s", filename);
    uint64_t fsize = noSDL_fileGetSize(filename);
    if (fsize == 0)
        return -1;

    sprintf(deb, "RAMDISK: allocating HIGH %lu bytes", fsize);
    noSDL_Kernel_Log(deb);

    void *rd = noSDL_HighMem_Alloc(fsize);
    if (rd != NULL)
    {
        sprintf(deb, "RAMDISK: reading %s", filename);
        noSDL_Kernel_Log(deb);

        uint64_t rdb = noSDL_fileFullRead(filename, rd, fsize);
        if (rdb != fsize)
        {
            sprintf(deb, "RAMDISK: read failed: %lu.", rdb);
            noSDL_Kernel_Log(deb);

            noSDL_HighMem_Delete(rd);
            return -1;
        }

        sprintf(deb, "RAMDISK: completed.");
        noSDL_Kernel_Log(deb);
    }
    else
    {
        sprintf(deb, "RAMDISK: cannot allocate HIGH ram buffer.");
        noSDL_Kernel_Log(deb);

        return -1;
    }

    int fd;
    if (!info->modify_backing_file)
        fd = open(filename, O_RDONLY | O_BINARY);
    else
        fd = open(filename, O_RDWR | O_BINARY);
    if (fd < 0)
        return -1;

    struct ramdisk_driver* sync_info = malloc(sizeof(struct ramdisk_driver));
    info->data = sync_info;
    sync_info->ramdisk = rd;
    sync_info->image_size = fsize;
    sync_info->block_size = BLOCK_SIZE;
    sync_info->block_array_size = (fsize + sync_info->block_size - 1) / sync_info->block_size;
    sync_info->blocks = calloc(sizeof(uint8_t*), sync_info->block_array_size);

    sync_info->raw_file_access = info->modify_backing_file;

    info->read = drive_ramdisk_read;
    info->state = drive_ramdisk_state;
    info->write = drive_ramdisk_write;
    info->prefetch = drive_ramdisk_prefetch;

    // Now determine drive geometry
    info->sectors = fsize / 512;
    info->sectors_per_cylinder = 63;
    info->heads = 16;
    info->cylinders_per_head = info->sectors / (info->sectors_per_cylinder * info->heads);

    sprintf(deb, "RAMDISK: OK.");
    noSDL_Kernel_Log(deb);

    return 0;
}

void drive_destroy_ramdisk(struct drive_info* info)
{
    struct ramdisk_driver* simple_info = info->data;
    noSDL_HighMem_Delete(simple_info->ramdisk);
    free(simple_info);
}

// Autodetect drive type
int drive_autodetect_type(char* path)
{
    struct stat statbuf;

    // Check for URL, unsupported here
    if (strstr(path, "http://") != NULL || strstr(path, "https://") != NULL)
        return -1;
    
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    if(fstat(fd, &statbuf)) {
        close(fd);
        return -1;
    }
    close(fd);
    if(S_ISDIR(statbuf.st_mode))
        return -1; // Chunked file, unsupported
    else
        return 1; // Raw image file 

}

