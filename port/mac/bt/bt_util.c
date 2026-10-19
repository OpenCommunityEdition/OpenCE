/* Byte helpers, the CRC-32 and the clock of the Bluetooth stack (bt_stack.h).
   Everything is read and written a byte at a time. */
#include "bt_stack.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

unsigned bt_get16(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

unsigned bt_get24(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16);
}

unsigned bt_get32(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

void bt_put16(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)(value & 0xff);
    p[1] = (unsigned char)((value >> 8) & 0xff);
}

void bt_put32(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)(value & 0xff);
    p[1] = (unsigned char)((value >> 8) & 0xff);
    p[2] = (unsigned char)((value >> 16) & 0xff);
    p[3] = (unsigned char)((value >> 24) & 0xff);
}

/* (the CRC is shared with the system Bluetooth path: mac_gamepad_report.c) */
unsigned bt_crc32_update(unsigned crc, const unsigned char *data, unsigned size)
{
    return mac_pad_crc32_update(crc, data, size);
}

unsigned bt_crc32_report(unsigned char header, const unsigned char *data, unsigned size)
{
    return mac_pad_crc32_report(header, data, size);
}

void bt_addr_text(const unsigned char *a, char *out)
{
    sprintf(out, "%02x:%02x:%02x:%02x:%02x:%02x", a[5], a[4], a[3], a[2], a[1], a[0]);
}

int bt_addr_parse(const char *text, unsigned char *addr)
{
    unsigned v[6];
    int i;
    if (sscanf(text, "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) return -1;
    for (i = 0; i < 6; i++) {
        if (v[i] > 255) return -1;
        addr[5 - i] = (unsigned char)v[i];
    }
    return 0;
}

double bt_now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

struct bt_stack *bt_stack_create(void)
{
    struct bt_stack *st = (struct bt_stack *)calloc(1, sizeof(*st));
    if (!st) return NULL;
    st->name_pending = -1;
    st->next_ident = 1;
    st->ncmd = 1;
    st->acl_max = 255;
    st->acl_credits = 4;
    st->link_handle = 0xffff;
    st->pad.battery_percent = -1;
    return st;
}

void bt_stack_destroy(struct bt_stack *st)
{
    free(st);
}

void bt_log(struct bt_stack *st, const char *format, ...)
{
    char text[600];
    va_list args;
    if (!st->log) return;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    st->log(st->log_ctx, text);
}

void bt_say(struct bt_stack *st, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(st->message, sizeof(st->message), format, args);
    va_end(args);
    bt_log(st, "%s", st->message);
}
