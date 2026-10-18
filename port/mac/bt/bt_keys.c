/* The link-key file of the Bluetooth stack (bt_stack.h): one line a pad,
     <dongle address> <pad address> <32 hex digits of key> <key type> <kind> <name>
   Lines of other dongles are kept as they are: a key belongs to the dongle that
   paired. The file holds secrets, so it is mode 0600 and replaced through a
   temporary file. */
#include "bt_stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define LINE 400

static int hex_value(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* parses a line; returns 1 if it is one */
static int parse_line(const char *line, unsigned char *local, struct bt_paired *k)
{
    char a1[40], a2[40], hex[80];
    int type = 0, kind = 0, used = 0, i;
    const char *name;
    if (sscanf(line, "%39s %39s %79s %d %d %n", a1, a2, hex, &type, &kind, &used) < 5 || used <= 0) return 0;
    if (strlen(hex) != 32) return 0;
    if (bt_addr_parse(a1, local) < 0 || bt_addr_parse(a2, k->addr) < 0) return 0;
    for (i = 0; i < 16; i++) {
        int hi = hex_value(hex[2 * i]), lo = hex_value(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) return 0;
        k->key[i] = (unsigned char)(hi * 16 + lo);
    }
    k->key_type = type;
    k->kind = kind;
    name = line + used;
    memset(k->name, 0, sizeof(k->name));
    strncpy(k->name, name, sizeof(k->name) - 1);
    for (i = 0; k->name[i]; i++)
        if (k->name[i] == '\n' || k->name[i] == '\r') k->name[i] = 0;
    return 1;
}

int bt_keys_load(const char *path, const unsigned char *local_addr, struct bt_paired *out, int max)
{
    FILE *f = fopen(path, "r");
    char line[LINE];
    int count = 0;
    if (!f) return 0;
    while (fgets(line, sizeof(line), f) && count < max) {
        unsigned char local[6];
        struct bt_paired k;
        if (parse_line(line, local, &k) && !memcmp(local, local_addr, 6)) out[count++] = k;
    }
    fclose(f);
    return count;
}

/* rewrites the file without the lines of this dongle and pad, then appends key (or not) */
static int rewrite(const char *path, const unsigned char *local_addr, const unsigned char *addr,
    const struct bt_paired *add)
{
    char tmp[600], line[LINE];
    FILE *in, *out;
    int i;
    if (strlen(path) + 5 >= sizeof(tmp)) return -1;
    sprintf(tmp, "%s.tmp", path);
    out = fopen(tmp, "w");
    if (!out) return -1;
    chmod(tmp, 0600);
    in = fopen(path, "r");
    if (in) {
        while (fgets(line, sizeof(line), in)) {
            unsigned char local[6];
            struct bt_paired k;
            if (parse_line(line, local, &k) && !memcmp(local, local_addr, 6) && !memcmp(k.addr, addr, 6))
                continue;
            fputs(line, out);
            if (!strchr(line, '\n')) fputc('\n', out);
        }
        fclose(in);
    }
    if (add) {
        char a1[20], a2[20];
        bt_addr_text(local_addr, a1);
        bt_addr_text(add->addr, a2);
        fprintf(out, "%s %s ", a1, a2);
        for (i = 0; i < 16; i++) fprintf(out, "%02x", add->key[i]);
        fprintf(out, " %d %d %s\n", add->key_type, add->kind, add->name);
    }
    if (fclose(out) != 0) { unlink(tmp); return -1; }
    if (rename(tmp, path) != 0) { unlink(tmp); return -1; }
    return 0;
}

int bt_keys_store(const char *path, const unsigned char *local_addr, const struct bt_paired *key)
{
    return rewrite(path, local_addr, key->addr, key);
}

int bt_keys_remove(const char *path, const unsigned char *local_addr, const unsigned char *addr)
{
    return rewrite(path, local_addr, addr, NULL);
}
