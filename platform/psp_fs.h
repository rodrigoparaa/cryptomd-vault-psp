#ifndef PSP_FS_H
#define PSP_FS_H

#include "../core/app_state.h"

/* Scans directory (e.g. ef0:/DOCS or ms0:/DOCS) and populates app->files */
int psp_fs_scan_directory(app_state *app, const char *dir_path);

/* Reads entire file into buffer (up to max_len). Returns bytes read, or -1 on error. */
int psp_fs_read_file(const char *path, uint8_t *buf, size_t max_len, size_t *out_len);

/* Atomic write to target file (.tmp -> fsync -> rename). Returns 0 on success. */
int psp_fs_write_atomic(const char *path, const uint8_t *data, size_t len);

/* Secure Shredder: overwrites file sectors with random data, then zeroes, then deletes */
int psp_fs_shred_file(const char *path);

#endif /* PSP_FS_H */
