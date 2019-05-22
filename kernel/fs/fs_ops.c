/* This file use for NCTU OSDI course */
/* It's contants fat file system operators */

#include <inc/stdio.h>
#include <fs.h>
#include <fat/ff.h>
#include <diskio.h>

extern struct fs_dev fat_fs;

/*TODO: Lab7, fat level file operator.
 *       Implement below functions to support basic file system operators by using the elmfat's API(f_xxx).
 *       Reference: http://elm-chan.org/fsw/ff/00index_e.html (or under doc directory (doc/00index_e.html))
 *
 *  Call flow example:
 *        ┌──────────────┐
 *        │     open     │
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │   sys_open   │  file I/O system call interface
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │  file_open   │  VFS level file API
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║   fat_open   ║  fat level file operator
 *        ╚══════════════╝
 *               ↓
 *        ┌──────────────┐
 *        │    f_open    │  FAT File System Module
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    diskio    │  low level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │     disk     │  simple ATA disk dirver
 *        └──────────────┘
 */

/* Note: 1. Get FATFS object from fs->data
*        2. Check fs->path parameter then call f_mount.
*/
int fat_mount(struct fs_dev *fs, const void* data) {
    return -f_mount(fs->data, fs->path, 1);
}

/* Note: Just call f_mkfs at root path '/' */
int fat_mkfs(const char* device_name) {
    return -f_mkfs("/", 0, 0);
}

/* Note: Convert the POSIX's open flag to elmfat's flag.
*        Example: if file->flags == O_RDONLY then open_mode = FA_READ
*                 if file->flags & O_APPEND then f_seek the file to end after f_open
*/
int fat_open(struct fs_fd* file) {
    int flag = 0;
    if(file->flags == O_RDONLY)
        flag |= FA_READ;
    if(file->flags & O_WRONLY)
        flag |= FA_WRITE;
    if(file->flags & O_RDWR)
        flag |= FA_READ | FA_WRITE;
    if((file->flags & O_CREAT) && !(file->flags & O_TRUNC))
        flag |= FA_CREATE_NEW;
    if(file->flags & O_TRUNC)
        flag |= FA_CREATE_ALWAYS;

    int retval = f_open(file->data, file->path, flag);
    if(file->flags & O_APPEND)
        fat_lseek(file, ((FIL*)file->data)->obj.objsize);
    return -retval;
}

int fat_close(struct fs_fd* file) {
    return -f_close(file->data);
}

int fat_read(struct fs_fd* file, void* buf, size_t count) {
    unsigned int len;
    int retval = f_read(file->data, buf, count, &len);
    if (retval)
        return -retval;

    file->pos += len;
    return len;
}

int fat_write(struct fs_fd* file, const void* buf, size_t count) {
    unsigned int len;
    int retval = f_write(file->data, buf, count, &len);
    if (retval)
        return -retval;

    file->pos += len;
    if (file->pos > file->size)
        file->size = file->pos;
    return len;
}

int fat_lseek(struct fs_fd* file, off_t offset) {
    return -f_lseek(file->data, offset);
}

int fat_unlink(const char *pathname) {
    return -f_unlink(pathname);
}

int fat_opendir(DIR *dir, const char *pathname) {
    return -f_opendir(dir, pathname);
}

int fat_readdir(DIR *dir,  FILINFO *fno) {
    return -f_readdir(dir, fno);
}

int fat_closedir(DIR *dir) {
    return -f_closedir(dir);
}

int fat_stat(const char *pathname, FILINFO *fno) {
    return -f_stat(pathname, fno);
}

struct fs_ops elmfat_ops = {
    .dev_name = "elmfat",
    .mount = fat_mount,
    .mkfs = fat_mkfs,
    .open = fat_open,
    .close = fat_close,
    .read = fat_read,
    .write = fat_write,
    .lseek = fat_lseek,
    .unlink = fat_unlink,
    .opendir = fat_opendir,
    .readdir = fat_readdir,
    .closedir = fat_closedir,
    .stat = fat_stat
};
