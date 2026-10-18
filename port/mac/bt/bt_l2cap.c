/* L2CAP (bt_stack.h): ACL fragmenting and reassembly, the signalling channel
   and the dynamic channels of a HID device (PSM 0x11 control, 0x13 interrupt).
   Basic mode only. Everything from the air is length-checked. */
#include "bt_stack.h"

#include <string.h>

static struct bt_channel *by_local(struct bt_stack *st, unsigned cid)
{
    int i;
    for (i = 0; i < BT_MAX_CHANNELS; i++)
        if (st->channel[i].used && st->channel[i].local == cid) return &st->channel[i];
    return NULL;
}

struct bt_channel *bt_l2cap_by_psm(struct bt_stack *st, unsigned psm)
{
    int i;
    for (i = 0; i < BT_MAX_CHANNELS; i++)
        if (st->channel[i].used && st->channel[i].psm == psm) return &st->channel[i];
    return NULL;
}

static struct bt_channel *channel_new(struct bt_stack *st, unsigned psm)
{
    int i;
    for (i = 0; i < BT_MAX_CHANNELS; i++)
        if (!st->channel[i].used) {
            memset(&st->channel[i], 0, sizeof(st->channel[i]));
            st->channel[i].used = 1;
            st->channel[i].psm = psm;
            st->channel[i].local = 0x40 + (unsigned)i;
            return &st->channel[i];
        }
    return NULL;
}

void bt_l2cap_reset(struct bt_stack *st)
{
    int i;
    for (i = 0; i < BT_MAX_CHANNELS; i++) st->channel[i].used = 0;
    st->l2_want = 0;
    st->l2_have = 0;
}

static void acl_write(struct bt_stack *st, unsigned handle, unsigned pb, const unsigned char *data, unsigned size)
{
    unsigned char packet[BT_PACKET_MAX];
    if (size + 4 > sizeof(packet)) return;
    packet[0] = (unsigned char)(handle & 0xff);
    packet[1] = (unsigned char)(((handle >> 8) & 0x0f) | (pb << 4));
    packet[2] = (unsigned char)(size & 0xff);
    packet[3] = (unsigned char)(size >> 8);
    memcpy(packet + 4, data, size);
    if (st->tr.send_acl(st->tr.ctx, packet, size + 4) != 0) bt_log(st, "ACL write failed");
    st->acl_credits--;
}

void bt_l2cap_send(struct bt_stack *st, unsigned cid, const unsigned char *data, unsigned size)
{
    unsigned char packet[BT_PACKET_MAX];
    unsigned sent = 0, first = 1;
    if (!st->link_up || size + 4 > sizeof(packet) - 4) return;
    bt_put16(packet, size);
    bt_put16(packet + 2, cid);
    memcpy(packet + 4, data, size);
    size += 4;
    while (sent < size) {
        unsigned chunk = size - sent;
        if (chunk > st->acl_max) chunk = st->acl_max;
        acl_write(st, st->link_handle, first ? 0 : 1, packet + sent, chunk);
        sent += chunk;
        first = 0;
    }
}

static unsigned ident_next(struct bt_stack *st)
{
    unsigned i = st->next_ident++;
    if (st->next_ident > 250) st->next_ident = 1;
    return i;
}

static void sig_send(struct bt_stack *st, unsigned code, unsigned ident, const unsigned char *data, unsigned size)
{
    unsigned char b[72];
    if (size > 64) return;
    b[0] = (unsigned char)code;
    b[1] = (unsigned char)ident;
    bt_put16(b + 2, size);
    if (size) memcpy(b + 4, data, size);
    bt_l2cap_send(st, 1, b, size + 4);
}

static void send_config_request(struct bt_stack *st, struct bt_channel *c)
{
    unsigned char d[8];
    bt_put16(d, c->remote);
    d[2] = 0; d[3] = 0;          /* flags */
    d[4] = 1; d[5] = 2;          /* option: MTU */
    d[6] = 0xa0; d[7] = 0x02;    /* 672 */
    sig_send(st, 4, ident_next(st), d, 8);
}

void bt_l2cap_connect(struct bt_stack *st, unsigned psm)
{
    unsigned char d[4];
    struct bt_channel *c = channel_new(st, psm);
    if (!c) { bt_log(st, "L2CAP: no free channel"); return; }
    c->state = 1;
    bt_put16(d, psm);
    bt_put16(d + 2, c->local);
    bt_log(st, "L2CAP: connection request PSM 0x%04x, local CID 0x%04x", psm, c->local);
    sig_send(st, 2, ident_next(st), d, 4);
}

void bt_l2cap_disconnect_all(struct bt_stack *st)
{
    int i;
    for (i = 0; i < BT_MAX_CHANNELS; i++)
        if (st->channel[i].used && st->channel[i].state == 3) {
            unsigned char d[4];
            bt_put16(d, st->channel[i].remote);
            bt_put16(d + 2, st->channel[i].local);
            sig_send(st, 6, ident_next(st), d, 4);
        }
}

static void check_open(struct bt_stack *st, struct bt_channel *c)
{
    if (c->state == 2 && c->cfg_in && c->cfg_out) {
        c->state = 3;
        bt_log(st, "L2CAP: channel PSM 0x%04x open (local 0x%04x, remote 0x%04x)", c->psm, c->local, c->remote);
        bt_hid_channel_open(st, c);
    }
}

static void signalling(struct bt_stack *st, const unsigned char *d, unsigned size)
{
    unsigned off = 0;
    while (off + 4 <= size) {
        unsigned code = d[off], ident = d[off + 1], len = bt_get16(d + off + 2);
        const unsigned char *p = d + off + 4;
        if (off + 4 + len > size) { bt_log(st, "L2CAP: truncated signalling command"); return; }
        if (code == 2 && len >= 4) {                    /* connection request */
            unsigned psm = bt_get16(p);
            unsigned char r[8];
            struct bt_channel *c = NULL;
            bt_log(st, "L2CAP: incoming connection request PSM 0x%04x from CID 0x%04x", psm, bt_get16(p + 2));
            if (psm == 0x11 || psm == 0x13) {
                c = bt_l2cap_by_psm(st, psm);
                if (c && c->state != 1) c = NULL;
                if (!c) c = channel_new(st, psm);
            }
            if (c) {
                c->remote = bt_get16(p + 2);
                c->state = 2;
                bt_put16(r, c->local);
                r[2] = p[2]; r[3] = p[3];
                r[4] = 0; r[5] = 0; r[6] = 0; r[7] = 0;
                sig_send(st, 3, ident, r, 8);
                send_config_request(st, c);
            } else {
                r[0] = 0; r[1] = 0; r[2] = p[2]; r[3] = p[3]; r[4] = 2; r[5] = 0; r[6] = 0; r[7] = 0; /* PSM not supported */
                sig_send(st, 3, ident, r, 8);
            }
        } else if (code == 3 && len >= 8) {             /* connection response */
            unsigned dcid = bt_get16(p), scid = bt_get16(p + 2), result = bt_get16(p + 4);
            struct bt_channel *c = by_local(st, scid);
            bt_log(st, "L2CAP: connection response dcid=0x%04x scid=0x%04x result=%u", dcid, scid, result);
            if (c && result == 0) {
                c->remote = dcid;
                c->state = 2;
                send_config_request(st, c);
            } else if (c && result != 1)
                c->used = 0;
        } else if (code == 4 && len >= 4) {             /* configuration request */
            unsigned dcid = bt_get16(p);
            struct bt_channel *c = by_local(st, dcid);
            unsigned o = 4;
            unsigned char r[6];
            if (!c) {
                bt_log(st, "L2CAP: config request for unknown CID 0x%04x", dcid);
            } else {
                while (o + 2 <= len) {
                    unsigned t = p[o] & 0x7f, l = p[o + 1];
                    if (o + 2 + l > len) break;
                    if (t == 1 && l == 2) c->mtu_out = bt_get16(p + o + 2);
                    o += 2 + l;
                }
                bt_put16(r, c->remote);
                r[2] = 0; r[3] = 0; r[4] = 0; r[5] = 0;
                sig_send(st, 5, ident, r, 6);
                c->cfg_in = 1;
                check_open(st, c);
            }
        } else if (code == 5 && len >= 6) {             /* configuration response */
            unsigned scid = bt_get16(p), result = bt_get16(p + 4);
            struct bt_channel *c = by_local(st, scid);
            bt_log(st, "L2CAP: config response CID 0x%04x result %u", scid, result);
            if (c && result == 0) {
                c->cfg_out = 1;
                check_open(st, c);
            }
        } else if (code == 6 && len >= 4) {             /* disconnection request */
            struct bt_channel *c = by_local(st, bt_get16(p));
            bt_log(st, "L2CAP: disconnect request for CID 0x%04x", bt_get16(p));
            sig_send(st, 7, ident, p, 4);
            if (c) c->used = 0;
        } else if (code == 8) {                         /* echo request */
            sig_send(st, 9, ident, p, len > 40 ? 40 : len);
        } else if (code == 10 && len >= 2) {            /* information request: not supported */
            unsigned char r[4];
            r[0] = p[0]; r[1] = p[1]; r[2] = 1; r[3] = 0;
            sig_send(st, 11, ident, r, 4);
        } else if (code == 1 || code == 7 || code == 9 || code == 11) {
            /* reject, disconnect response, echo response, information response */
        } else {
            unsigned char r[2];
            bt_log(st, "L2CAP: unknown signalling code 0x%02x", code);
            r[0] = 0; r[1] = 0;
            sig_send(st, 1, ident, r, 2);
        }
        off += 4 + len;
    }
}

static void l2cap_input(struct bt_stack *st, unsigned cid, const unsigned char *d, unsigned size)
{
    if (cid == 1) { signalling(st, d, size); return; }
    if (cid >= 0x40 && cid < 0x40 + BT_MAX_CHANNELS) {
        struct bt_channel *c = by_local(st, cid);
        if (c && c->state == 3) bt_hid_data(st, c, d, size);
    }
}

void bt_l2cap_input_acl(struct bt_stack *st, const unsigned char *p, unsigned size)
{
    unsigned pb, dlen;
    const unsigned char *d;
    if (size < 4) return;
    pb = (p[1] >> 4) & 3;
    dlen = bt_get16(p + 2);
    if (dlen + 4 > size) { bt_log(st, "ACL: short packet"); return; }
    d = p + 4;
    if (pb == 2 || pb == 0) {
        if (dlen < 4) return;
        st->l2_want = bt_get16(d) + 4;
        st->l2_cid = bt_get16(d + 2);
        if (st->l2_want > sizeof(st->l2_buf)) { bt_log(st, "ACL: oversize L2CAP packet %u", st->l2_want); st->l2_want = 0; return; }
        st->l2_have = 0;
    } else if (pb != 1 || !st->l2_want)
        return;
    if (st->l2_have + dlen > sizeof(st->l2_buf)) { st->l2_want = 0; return; }
    memcpy(st->l2_buf + st->l2_have, d, dlen);
    st->l2_have += dlen;
    if (st->l2_have >= st->l2_want) {
        unsigned want = st->l2_want;
        st->l2_want = 0;
        l2cap_input(st, st->l2_cid, st->l2_buf + 4, want - 4);
    }
}
