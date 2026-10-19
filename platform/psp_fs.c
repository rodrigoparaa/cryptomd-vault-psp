#include "psp_fs.h"
#include "psp_entropy.h"
#include "../core/pdoc_container.h"
#include "../core/cipher_chacha20.h"
#include "../core/crypto_primitives.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#ifdef __PSP__
#include <pspkernel.h>
#include <pspiofilemgr.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

int psp_fs_scan_directory(app_state *app, const char *dir_path) {
    if (!app || !dir_path) return -1;
    app->file_count = 0;
    app->selected_file_idx = 0;
    strncpy(app->current_dir, dir_path, MAX_PATH_LEN - 1);
    app->current_dir[MAX_PATH_LEN - 1] = '\0';

#ifdef __PSP__
    SceUID dfd = sceIoDopen(dir_path);
    if (dfd < 0) {
        /* If directory doesn't exist, try creating it */
        sceIoMkdir(dir_path, 0777);
        dfd = sceIoDopen(dir_path);
        if (dfd < 0) return -1;
    }

    SceIoDirent entry;
    memset(&entry, 0, sizeof(entry));

    while (sceIoDread(dfd, &entry) > 0 && app->file_count < MAX_FILES_LIST) {
        if (strcmp(entry.d_name, ".") == 0 || strcmp(entry.d_name, "..") == 0) continue;
        if (entry.d_name[0] == '.') continue; /* Skip hidden / AppleDouble */

        file_item *item = &app->files[app->file_count];
        snprintf(item->name, sizeof(item->name), "%.63s", entry.d_name);
        snprintf(item->full_path, sizeof(item->full_path), "%.180s/%.63s", dir_path, entry.d_name);
        item->size = (uint32_t)entry.d_stat.st_size;
        item->is_dir = FIO_SO_ISDIR(entry.d_stat.st_attr) ? 1 : 0;
        item->is_pdoc = 0;
        item->cipher_level = 0;

        if (!item->is_dir) {
            const char *dot = strrchr(entry.d_name, '.');
            if (dot) {
                if (strcasecmp(dot, ".pdoc") == 0) {
                    item->is_pdoc = 1;
                    /* Peek header to determine level */
                    SceUID fd = sceIoOpen(item->full_path, PSP_O_RDONLY, 0777);
                    if (fd >= 0) {
                        pdoc_header hdr;
                        if (sceIoRead(fd, &hdr, sizeof(hdr)) == sizeof(hdr)) {
                            if (pdoc_is_valid_header((const uint8_t *)&hdr, sizeof(hdr), NULL)) {
                                item->cipher_level = hdr.cipher_level;
                            }
                        }
                        sceIoClose(fd);
                    }
                } else if (strcasecmp(dot, ".md") == 0 || strcasecmp(dot, ".txt") == 0) {
                    /* Check if it has a visible header */
                    SceUID fd = sceIoOpen(item->full_path, PSP_O_RDONLY, 0777);
                    if (fd >= 0) {
                        char head[128];
                        int rd = sceIoRead(fd, head, sizeof(head) - 1);
                        if (rd > 0) {
                            head[rd] = '\0';
                            uint8_t v_lvl = 0;
                            if (pdoc_parse_visible_header(head, rd, &v_lvl, NULL, NULL, NULL)) {
                                item->is_pdoc = 1;
                                item->cipher_level = v_lvl;
                            }
                        }
                        sceIoClose(fd);
                    }
                }
            }
        }
        app->file_count++;
    }
    sceIoDclose(dfd);
#else
    DIR *d = opendir(dir_path);
    if (!d) return -1;
    struct dirent *de;
    while ((de = readdir(d)) != NULL && app->file_count < MAX_FILES_LIST) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        if (de->d_name[0] == '.') continue;

        file_item *item = &app->files[app->file_count];
        snprintf(item->name, sizeof(item->name), "%.63s", de->d_name);
        snprintf(item->full_path, sizeof(item->full_path), "%.180s/%.63s", dir_path, de->d_name);

        struct stat st;
        if (stat(item->full_path, &st) == 0) {
            item->size = (uint32_t)st.st_size;
            item->is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
        } else {
            item->size = 0;
            item->is_dir = 0;
        }

        item->is_pdoc = 0;
        item->cipher_level = 0;

        if (!item->is_dir) {
            const char *dot = strrchr(de->d_name, '.');
            if (dot && strcasecmp(dot, ".pdoc") == 0) {
                item->is_pdoc = 1;
                FILE *f = fopen(item->full_path, "rb");
                if (f) {
                    pdoc_header hdr;
                    if (fread(&hdr, 1, sizeof(hdr), f) == sizeof(hdr)) {
                        if (pdoc_is_valid_header((const uint8_t *)&hdr, sizeof(hdr), NULL)) {
                            item->cipher_level = hdr.cipher_level;
                        }
                    }
                    fclose(f);
                }
            }
        }
        app->file_count++;
    }
    closedir(d);
#endif
    return 0;
}

int psp_fs_read_file(const char *path, uint8_t *buf, size_t max_len, size_t *out_len) {
    if (!path || !buf || max_len == 0 || !out_len) return -1;

#ifdef __PSP__
    SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
    if (fd < 0) return -1;
    int rd = sceIoRead(fd, buf, max_len);
    sceIoClose(fd);
    if (rd < 0) return -1;
    *out_len = (size_t)rd;
    return 0;
#else
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    size_t rd = fread(buf, 1, max_len, f);
    fclose(f);
    *out_len = rd;
    return 0;
#endif
}

int psp_fs_write_atomic(const char *path, const uint8_t *data, size_t len) {
    if (!path || (!data && len > 0)) return -1;

    char tmp_path[MAX_PATH_LEN + 8];
    snprintf(tmp_path, sizeof(tmp_path), "%.250s.tmp", path);

#ifdef __PSP__
    SceUID fd = sceIoOpen(tmp_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd < 0) return -1;
    int wr = sceIoWrite(fd, data, len);
    sceIoClose(fd);
    if (wr != (int)len) {
        sceIoRemove(tmp_path);
        return -1;
    }
    sceIoRemove(path);
    if (sceIoRename(tmp_path, path) < 0) return -1;
    return 0;
#else
    FILE *f = fopen(tmp_path, "wb");
    if (!f) return -1;
    size_t wr = fwrite(data, 1, len, f);
    fclose(f);
    if (wr != len) {
        remove(tmp_path);
        return -1;
    }
    remove(path);
    if (rename(tmp_path, path) != 0) return -1;
    return 0;
#endif
}

int psp_fs_shred_file(const char *path) {
    if (!path) return -1;

#ifdef __PSP__
    SceUID fd = sceIoOpen(path, PSP_O_WRONLY, 0777);
    if (fd < 0) {
        return sceIoRemove(path);
    }
    SceOff size = sceIoLseek(fd, 0, PSP_SEEK_END);
    sceIoLseek(fd, 0, PSP_SEEK_SET);

    if (size > 0) {
        uint8_t noise[512];
        SceOff remaining = size;
        while (remaining > 0) {
            size_t chunk = (remaining > (SceOff)sizeof(noise)) ? sizeof(noise) : (size_t)remaining;
            psp_entropy_gather(noise, chunk);
            sceIoWrite(fd, noise, chunk);
            remaining -= chunk;
        }

        /* Pass 2: zeroes */
        crypto_zeroize(noise, sizeof(noise));
        sceIoLseek(fd, 0, PSP_SEEK_SET);
        remaining = size;
        while (remaining > 0) {
            size_t chunk = (remaining > (SceOff)sizeof(noise)) ? sizeof(noise) : (size_t)remaining;
            sceIoWrite(fd, noise, chunk);
            remaining -= chunk;
        }
    }
    sceIoClose(fd);
    return sceIoRemove(path);
#else
    FILE *f = fopen(path, "r+b");
    if (!f) return remove(path);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz > 0) {
        uint8_t noise[512];
        long remaining = sz;
        while (remaining > 0) {
            size_t chunk = (remaining > (long)sizeof(noise)) ? sizeof(noise) : (size_t)remaining;
            psp_entropy_gather(noise, chunk);
            fwrite(noise, 1, chunk, f);
            remaining -= chunk;
        }
        fflush(f);
        fseek(f, 0, SEEK_SET);
        crypto_zeroize(noise, sizeof(noise));
        remaining = sz;
        while (remaining > 0) {
            size_t chunk = (remaining > (long)sizeof(noise)) ? sizeof(noise) : (size_t)remaining;
            fwrite(noise, 1, chunk, f);
            remaining -= chunk;
        }
        fflush(f);
    }
    fclose(f);
    return remove(path);
#endif
}
