/* The Realtek firmware loader (bt_stack.h). An RTL8761B comes up as a ROM
   that answers the basic commands but finds no other device; a patch loaded into
   its RAM makes it a full BR/EDR and LE controller until it loses power.

   The firmware file (rtl8761bu_fw.bin) begins with "Realtech", a version word
   (4 bytes at offset 8), a patch count (2 bytes at 12), then three tables of
   that many entries: chip ids (2 bytes each), patch lengths (2 each) and patch
   offsets (4 each). The patch for a ROM is the one whose chip id is the ROM
   version (vendor command 0xFC6D) plus one. The image to download is that patch
   with its last four bytes replaced by the file's version word, followed by the
   config file (rtl8761bu_config.bin, 6 bytes: 55 ab 23 87 00 00). It goes in
   fragments of 252 bytes through vendor command 0xFC20: a one-byte index (0 to
   0x7F, then 1 to 0x7F again; bit 7 on the last fragment) and the data. The
   format was read from public descriptions of the Linux btrtl driver and
   confirmed on the dongle; the file itself is Realtek's and is not part of this
   repository. */
#include "bt_stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAGMENT 252

static const unsigned char default_config[6] = { 0x55, 0xab, 0x23, 0x87, 0x00, 0x00 };

static unsigned char *slurp(const char *path, unsigned *size)
{
    FILE *f = fopen(path, "rb");
    unsigned char *buf;
    long n;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > 1048576) { fclose(f); return NULL; }
    buf = (unsigned char *)malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(buf); return NULL; }
    fclose(f);
    *size = (unsigned)n;
    return buf;
}

unsigned bt_realtek_image(const unsigned char *fw, unsigned fw_size, const unsigned char *cfg,
    unsigned cfg_size, unsigned rom_version, unsigned char **image)
{
    unsigned num, i, patch_len = 0, patch_off = 0, total;
    unsigned char *out;
    *image = NULL;
    if (fw_size < 20 || memcmp(fw, "Realtech", 8) != 0) return 0;
    num = bt_get16(fw + 12);
    if (num == 0 || fw_size < 14 + 8 * num) return 0;
    for (i = 0; i < num; i++) {
        unsigned chip = bt_get16(fw + 14 + 2 * i);
        unsigned len = bt_get16(fw + 14 + 2 * num + 2 * i);
        unsigned off = bt_get32(fw + 14 + 4 * num + 4 * i);
        if (chip == rom_version + 1) { patch_len = len; patch_off = off; }
    }
    if (!patch_off || patch_len < 4 || patch_off > fw_size || patch_len > fw_size - patch_off) return 0;
    if (!cfg || !cfg_size) { cfg = default_config; cfg_size = sizeof(default_config); }
    if (cfg_size > 4096) return 0;
    total = patch_len + cfg_size;
    out = (unsigned char *)malloc(total);
    if (!out) return 0;
    memcpy(out, fw + patch_off, patch_len - 4);
    memcpy(out + patch_len - 4, fw + 8, 4);
    memcpy(out + patch_len, cfg, cfg_size);
    *image = out;
    return total;
}

int bt_realtek_load(struct bt_stack *st, unsigned rom_version)
{
    unsigned fw_size = 0, cfg_size = 0, total, frags, f, index = 0;
    unsigned char *fw, *cfg = NULL, *image = NULL;
    double started = bt_now();
    fw = slurp(st->firmware_path, &fw_size);
    if (!fw) {
        bt_say(st, "Cannot read the firmware file %s", st->firmware_path);
        return -1;
    }
    if (st->config_path[0]) cfg = slurp(st->config_path, &cfg_size);
    total = bt_realtek_image(fw, fw_size, cfg, cfg_size, rom_version, &image);
    free(fw);
    free(cfg);
    if (!total) {
        bt_say(st, "The firmware file has no patch for this chip (ROM version %u)", rom_version);
        return -1;
    }
    frags = total / FRAGMENT + 1;
    for (f = 0; f < frags; f++) {
        unsigned char cmd[FRAGMENT + 1];
        unsigned len = FRAGMENT;
        unsigned char ret[64];
        unsigned rs;
        int status;
        cmd[0] = (unsigned char)index++;
        if (cmd[0] == 0x7f) index = 1;
        if (f == frags - 1) {
            cmd[0] |= 0x80;
            len = total % FRAGMENT;
        }
        if (len) memcpy(cmd + 1, image + f * FRAGMENT, len);
        status = bt_hci_sync(st, 0xfc20, cmd, len + 1, ret, &rs, 3.0);
        if (status != 0) {
            bt_say(st, "Firmware download failed at fragment %u of %u (status %d)", f, frags, status);
            free(image);
            return -1;
        }
    }
    bt_log(st, "firmware downloaded: %u bytes in %u fragments, %.2f s", total, frags, bt_now() - started);
    free(image);
    return 0;
}
