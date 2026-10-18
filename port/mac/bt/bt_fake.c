/* A fake dongle for tests (bt_stack.h): answers like a Realtek RTL8761BU
   with a DualSense in range and in pairing mode, so the stack can be run
   without hardware, on any machine. It models only what the stack does:
   start-up, firmware download, inquiry, name, connection, Secure Simple
   Pairing, L2CAP channels 0x11 and 0x13, the feature report and a stream of
   full input reports. */
#include "bt_stack.h"

#include <stdlib.h>
#include <string.h>

#define QUEUE 128

struct fake_packet {
    int kind;
    unsigned size;
    unsigned char data[300];
};

struct bt_fake {
    struct fake_packet queue[QUEUE];
    unsigned head, tail;
    int rom_only;
    int patched;
    unsigned seen[0x10000 >> 4];      /* commands seen, by opcode / 16 (coarse) */
    unsigned seen_exact[64][2];
    int seen_count;
    unsigned char pad_addr[6];
    int link;
    unsigned cid_control, cid_interrupt;        /* the pad's CIDs (the stack's are 0x40 and 0x41) */
    unsigned stack_cid_control, stack_cid_interrupt;
    int streaming;
    unsigned sequence;
    unsigned char last_output[80];
    unsigned last_output_size;
    unsigned stream_budget;
};

static void queue_packet(struct bt_fake *f, int kind, const unsigned char *d, unsigned n)
{
    unsigned next = (f->head + 1) % QUEUE;
    if (next == f->tail || n > sizeof(f->queue[0].data)) return;
    f->queue[f->head].kind = kind;
    f->queue[f->head].size = n;
    memcpy(f->queue[f->head].data, d, n);
    f->head = next;
}

static void event(struct bt_fake *f, unsigned code, const unsigned char *params, unsigned n)
{
    unsigned char e[270];
    e[0] = (unsigned char)code;
    e[1] = (unsigned char)n;
    if (n) memcpy(e + 2, params, n);
    queue_packet(f, BT_PACKET_EVENT, e, n + 2);
}

static void complete(struct bt_fake *f, unsigned opcode, const unsigned char *ret, unsigned n)
{
    unsigned char p[270];
    p[0] = 1;
    bt_put16(p + 1, opcode);
    p[3] = 0;
    if (n) memcpy(p + 4, ret, n);
    event(f, 0x0e, p, 4 + n);
}

static void status(struct bt_fake *f, unsigned opcode)
{
    unsigned char p[4];
    p[0] = 0; p[1] = 1;
    bt_put16(p + 2, opcode);
    event(f, 0x0f, p, 4);
}

static void acl(struct bt_fake *f, unsigned cid, const unsigned char *d, unsigned n)
{
    unsigned char p[300];
    bt_put16(p, 0x2001);            /* handle 1, first fragment */
    bt_put16(p + 2, n + 4);
    bt_put16(p + 4, n);
    bt_put16(p + 6, cid);
    memcpy(p + 8, d, n);
    queue_packet(f, BT_PACKET_ACL, p, n + 8);
}

static void signal_out(struct bt_fake *f, unsigned code, unsigned ident, const unsigned char *d, unsigned n)
{
    unsigned char b[40];
    b[0] = (unsigned char)code; b[1] = (unsigned char)ident;
    bt_put16(b + 2, n);
    memcpy(b + 4, d, n);
    acl(f, 1, b, n + 4);
}

static void input_report(struct bt_fake *f)
{
    unsigned char r[80];
    memset(r, 0, sizeof(r));
    r[0] = 0xa1;
    r[1] = 0x31;
    r[2] = (unsigned char)(f->sequence++ << 4);
    r[3] = 10; r[4] = 200; r[5] = 128; r[6] = 129;     /* sticks */
    r[7] = 3; r[8] = 255;                              /* L2 3, R2 255 */
    r[10] = 0x28;                                      /* hat none, cross */
    r[11] = 0x02;                                      /* R1 */
    r[12] = 0x02;                                      /* touchpad click */
    r[35] = 0x00; r[36] = 0x10; r[37] = 0x71; r[38] = 0x20; /* finger 1: x 0x110, y 0x207 */
    r[39] = 0x80;                                      /* finger 2 up */
    r[55] = 0x07;                                      /* battery level 7 */
    bt_put32(r + 75, bt_crc32_report(0xa1, r + 1, 74));
    acl(f, f->stack_cid_interrupt, r, 79);
}

static void note(struct bt_fake *f, unsigned opcode)
{
    int i;
    for (i = 0; i < f->seen_count; i++)
        if (f->seen_exact[i][0] == opcode) { f->seen_exact[i][1]++; return; }
    if (f->seen_count < 64) {
        f->seen_exact[f->seen_count][0] = opcode;
        f->seen_exact[f->seen_count][1] = 1;
        f->seen_count++;
    }
}

unsigned bt_fake_commands_seen(const struct bt_fake *f, unsigned opcode)
{
    int i;
    for (i = 0; i < f->seen_count; i++)
        if (f->seen_exact[i][0] == opcode) return f->seen_exact[i][1];
    return 0;
}

int bt_fake_output_report(const struct bt_fake *f, unsigned char *out, unsigned cap)
{
    if (!f->last_output_size || f->last_output_size > cap) return 0;
    memcpy(out, f->last_output, f->last_output_size);
    return (int)f->last_output_size;
}

static int fake_command(void *ctx, const unsigned char *packet, unsigned size)
{
    struct bt_fake *f = (struct bt_fake *)ctx;
    unsigned opcode, n;
    const unsigned char *p;
    unsigned char r[300];
    if (size < 3) return -1;
    opcode = bt_get16(packet);
    n = packet[2];
    p = packet + 3;
    if (size != 3 + n) return -1;
    note(f, opcode);
    switch (opcode) {
    case 0x1001:
        memset(r, 0, 9);
        r[0] = 0x0a;
        if (f->rom_only && !f->patched) { bt_put16(r + 1, 0x000b); bt_put16(r + 6, 0x8761); }
        else { bt_put16(r + 1, 0xdfc6); bt_put16(r + 6, 0xd922); }
        r[3] = 0x0a; r[4] = 0x5d;
        complete(f, opcode, r, 8);
        break;
    case 0xfc6d:
        r[0] = 1;
        complete(f, opcode, r, 1);
        break;
    case 0xfc20:
        if (n >= 1 && (p[0] & 0x80)) f->patched = 1;
        complete(f, opcode, r, 0);
        break;
    case 0x1005:
        bt_put16(r, 1000); r[2] = 255; bt_put16(r + 3, 6); bt_put16(r + 5, 0);
        complete(f, opcode, r, 7);
        break;
    case 0x1009:
        r[0] = 0xd8; r[1] = 0x15; r[2] = 0x8d; r[3] = 0x5d; r[4] = 0xe1; r[5] = 0x20;
        complete(f, opcode, r, 6);
        break;
    case 0x0401: { /* Inquiry: one result with RSSI, then complete */
        status(f, opcode);
        r[0] = 1;
        memcpy(r + 1, f->pad_addr, 6);
        r[7] = 1; r[8] = 0;
        r[9] = 0x08; r[10] = 0x25; r[11] = 0x00;        /* class 0x002508 */
        bt_put16(r + 12, 0x1234);
        r[14] = (unsigned char)-44;
        event(f, 0x22, r, 15);
        r[0] = 0;
        event(f, 0x01, r, 1);
        break;
    }
    case 0x0419:
        status(f, opcode);
        r[0] = 0;
        memcpy(r + 1, p, 6);
        memset(r + 7, 0, 248);
        strcpy((char *)r + 7, "DualSense Wireless Controller");
        event(f, 0x07, r, 255);
        break;
    case 0x0405:
        status(f, opcode);
        r[0] = 0; bt_put16(r + 1, 1);
        memcpy(r + 3, p, 6);
        r[9] = 1; r[10] = 0;
        event(f, 0x03, r, 11);
        f->link = 1;
        break;
    case 0x0411:
        status(f, opcode);
        memcpy(r, f->pad_addr, 6);
        event(f, 0x31, r, 6);
        break;
    case 0x042b: /* IO_Capability_Request_Reply */
        memcpy(r, f->pad_addr, 6);
        complete(f, opcode, r, 6);
        r[6] = 0x03; r[7] = 0; r[8] = 0x04;
        event(f, 0x32, r, 9);
        memcpy(r, f->pad_addr, 6);
        r[6] = 0; r[7] = 0; r[8] = 0; r[9] = 0;
        event(f, 0x33, r, 10);
        break;
    case 0x042c: { /* User_Confirmation_Request_Reply: pairing completes with a key */
        int i;
        memcpy(r, f->pad_addr, 6);
        complete(f, opcode, r, 6);
        r[0] = 0;
        memcpy(r + 1, f->pad_addr, 6);
        event(f, 0x36, r, 7);
        memcpy(r, f->pad_addr, 6);
        for (i = 0; i < 16; i++) r[6 + i] = (unsigned char)(0xa0 + i);
        r[22] = 4;
        event(f, 0x18, r, 23);
        r[0] = 0; bt_put16(r + 1, 1);
        event(f, 0x06, r, 3);
        break;
    }
    case 0x0413:
        status(f, opcode);
        r[0] = 0; bt_put16(r + 1, 1); r[3] = 1;
        event(f, 0x08, r, 4);
        break;
    case 0x0406:
        status(f, opcode);
        r[0] = 0; bt_put16(r + 1, 1); r[3] = 0x16;
        event(f, 0x05, r, 4);
        f->link = 0;
        f->streaming = 0;
        break;
    case 0x0402:
        complete(f, opcode, r, 0);
        break;
    default:
        complete(f, opcode, r, 0);
        break;
    }
    return 0;
}

static int fake_acl(void *ctx, const unsigned char *packet, unsigned size)
{
    struct bt_fake *f = (struct bt_fake *)ctx;
    unsigned cid, len;
    const unsigned char *d;
    if (size < 8) return -1;
    len = bt_get16(packet + 4);
    cid = bt_get16(packet + 6);
    d = packet + 8;
    if (len + 8 > size) return -1;
    if (cid == 1 && len >= 4) {
        unsigned code = d[0], ident = d[1];
        const unsigned char *s = d + 4;
        unsigned char r[16];
        if (code == 2) { /* connection request: accept, then ask to configure */
            unsigned psm = bt_get16(s), scid = bt_get16(s + 2);
            unsigned mine = psm == 0x11 ? 0x50 : 0x51;
            if (psm == 0x11) { f->stack_cid_control = scid; f->cid_control = mine; }
            else { f->stack_cid_interrupt = scid; f->cid_interrupt = mine; }
            bt_put16(r, mine); bt_put16(r + 2, scid); bt_put16(r + 4, 0); bt_put16(r + 6, 0);
            signal_out(f, 3, ident, r, 8);
            bt_put16(r, scid); bt_put16(r + 2, 0); r[4] = 1; r[5] = 2; bt_put16(r + 6, 672);
            signal_out(f, 4, 77, r, 8);
        } else if (code == 4) { /* configuration request: accept */
            bt_put16(r, bt_get16(s)); bt_put16(r + 2, 0); bt_put16(r + 4, 0);
            r[0] = (unsigned char)(bt_get16(s) == f->cid_control ? f->stack_cid_control : f->stack_cid_interrupt);
            r[1] = 0;
            signal_out(f, 5, ident, r, 6);
        }
    } else if (cid == f->cid_control && len >= 2 && (d[0] & 0xf0) == 0x40) {
        /* GET_REPORT(feature): answer with data, and start the full reports */
        unsigned char r[70];
        memset(r, 0, sizeof(r));
        r[0] = 0xa3; r[1] = d[1];
        acl(f, f->stack_cid_control, r, 41);
        f->streaming = 1;
        f->stream_budget = 50;
    } else if (cid == f->cid_interrupt && len >= 2 && d[0] == 0xa2) {
        memcpy(f->last_output, d, len > sizeof(f->last_output) ? sizeof(f->last_output) : len);
        f->last_output_size = len > sizeof(f->last_output) ? (unsigned)sizeof(f->last_output) : len;
    }
    return 0;
}

static int fake_receive(void *ctx, unsigned char *buf, unsigned cap, unsigned *size)
{
    struct bt_fake *f = (struct bt_fake *)ctx;
    int kind;
    if (f->tail == f->head) return BT_PACKET_NONE;
    kind = f->queue[f->tail].kind;
    *size = f->queue[f->tail].size;
    if (*size > cap) *size = cap;
    memcpy(buf, f->queue[f->tail].data, *size);
    f->tail = (f->tail + 1) % QUEUE;
    return kind;
}

static void fake_wait(void *ctx, double seconds)
{
    struct bt_fake *f = (struct bt_fake *)ctx;
    (void)seconds;
    if (f->streaming && f->stream_budget) {
        f->stream_budget--;
        input_report(f);
    }
}

static int fake_failed(void *ctx) { (void)ctx; return 0; }
static void fake_close(void *ctx) { (void)ctx; }

struct bt_fake *bt_fake_create(struct bt_stack *st, int rom_only)
{
    struct bt_fake *f = (struct bt_fake *)calloc(1, sizeof(*f));
    if (!f) return NULL;
    f->rom_only = rom_only;
    bt_addr_parse("4c:b9:9b:fd:1d:81", f->pad_addr);
    st->tr.ctx = f;
    st->tr.send_command = fake_command;
    st->tr.send_acl = fake_acl;
    st->tr.receive = fake_receive;
    st->tr.wait = fake_wait;
    st->tr.failed = fake_failed;
    st->tr.close = fake_close;
    return f;
}

void bt_fake_destroy(struct bt_fake *f)
{
    free(f);
}
