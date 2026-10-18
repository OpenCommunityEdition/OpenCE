/* HCI (bt_stack.h): the command queue, the controller's start-up, discovery
   (inquiry and name requests), links, and the stack's top-level requests.
   Parameters of events are read a byte at a time at the offsets of the
   Bluetooth Core Specification, Volume 4 Part E (e[0] is the event code, e[1]
   the length, the parameters start at e[2]). */
#include "bt_stack.h"

#include <stdio.h>
#include <string.h>

#define SEARCH_INQUIRY_LENGTH 5   /* units of 1.28 s */
#define NAME_TIMEOUT 6.0
#define PAIR_TIMEOUT 45.0
#define PAGE_INTERVAL 12.0

/* ---- commands ---- */

void bt_hci_command(struct bt_stack *st, unsigned opcode, const unsigned char *params, unsigned count)
{
    unsigned next = (st->queue_head + 1) % BT_COMMAND_QUEUE;
    if (next == st->queue_tail) { bt_log(st, "HCI: command queue full, dropping 0x%04x", opcode); return; }
    if (count > sizeof(st->queue[0].params)) return;
    st->queue[st->queue_head].opcode = opcode;
    st->queue[st->queue_head].length = count;
    if (count) memcpy(st->queue[st->queue_head].params, params, count);
    st->queue_head = next;
}

void bt_hci_flush(struct bt_stack *st)
{
    if (st->ncmd <= 0 && st->queue_tail != st->queue_head && bt_now() - st->command_time > 2.0) {
        bt_log(st, "HCI: no command credit for 2 s, carrying on");
        st->ncmd = 1;
    }
    while (st->queue_tail != st->queue_head && st->ncmd > 0) {
        struct bt_command *c = &st->queue[st->queue_tail];
        unsigned char packet[270];
        bt_put16(packet, c->opcode);
        packet[2] = (unsigned char)c->length;
        if (c->length) memcpy(packet + 3, c->params, c->length);
        if (st->tr.send_command(st->tr.ctx, packet, 3 + c->length) != 0)
            bt_log(st, "HCI: command 0x%04x could not be sent", c->opcode);
        st->ncmd--;
        st->command_time = bt_now();
        st->queue_tail = (st->queue_tail + 1) % BT_COMMAND_QUEUE;
    }
}

int bt_hci_sync(struct bt_stack *st, unsigned opcode, const unsigned char *params, unsigned count,
    unsigned char *ret, unsigned *retlen, double timeout)
{
    double end = bt_now() + timeout;
    st->sync_opcode = opcode;
    st->sync_wait = 1;
    st->sync_done = 0;
    bt_hci_command(st, opcode, params, count);
    while (!st->sync_done && bt_now() < end && !st->failed) bt_stack_pump(st, 0.02);
    st->sync_wait = 0;
    if (!st->sync_done) { bt_log(st, "HCI: command 0x%04x timed out", opcode); return -1; }
    if (ret) memcpy(ret, st->sync_return, st->sync_length);
    if (retlen) *retlen = st->sync_length;
    return st->sync_status;
}

/* ---- pump ---- */

void bt_stack_pump(struct bt_stack *st, double seconds)
{
    double end = bt_now() + seconds;
    unsigned char buf[BT_PACKET_MAX];
    unsigned size;
    int kind;
    for (;;) {
        bt_hci_flush(st);
        kind = st->tr.receive(st->tr.ctx, buf, sizeof(buf), &size);
        if (kind == BT_PACKET_NONE) {
            double left = end - bt_now();
            if (left <= 0 || st->failed) break;
            st->tr.wait(st->tr.ctx, left > 0.02 ? 0.02 : left);
            if (st->tr.failed(st->tr.ctx)) { st->failed = 1; break; }
            kind = st->tr.receive(st->tr.ctx, buf, sizeof(buf), &size);
            if (kind == BT_PACKET_NONE) continue;
        }
        if (kind == BT_PACKET_EVENT) bt_hci_event(st, buf, size);
        else bt_l2cap_input_acl(st, buf, size);
        if (bt_now() >= end) break;
    }
    bt_hci_flush(st);
}

/* ---- the paired list and the found list ---- */

int bt_hci_is_paired(const struct bt_stack *st, const unsigned char *addr)
{
    int i;
    for (i = 0; i < st->paired_count; i++)
        if (!memcmp(st->paired[i].addr, addr, 6)) return 1;
    return 0;
}

struct bt_found *bt_hci_found_by_addr(struct bt_stack *st, const unsigned char *addr)
{
    int i;
    for (i = 0; i < st->found_count; i++)
        if (!memcmp(st->found[i].addr, addr, 6)) return &st->found[i];
    return NULL;
}

static void classify(struct bt_found *f)
{
    unsigned major = (f->cod >> 8) & 0x1f, minor = (f->cod >> 2) & 0x3f;
    f->gamepad = major == 5 && ((minor & 0x0f) == 1 || (minor & 0x0f) == 2);
    if (f->named) {
        f->kind = bt_pad_kind_from_name(f->name);
        f->supported = f->kind != BT_KIND_UNKNOWN;
        if (strstr(f->name, "Controller") || strstr(f->name, "Gamepad")) f->gamepad = 1;
    }
}

static void note_inquiry(struct bt_stack *st, const unsigned char *addr, unsigned pscan, unsigned cod,
    unsigned clock, int rssi)
{
    struct bt_found *f = bt_hci_found_by_addr(st, addr);
    if (!f) {
        char text[20];
        if (st->found_count >= BT_MAX_FOUND) return;
        f = &st->found[st->found_count++];
        memset(f, 0, sizeof(*f));
        memcpy(f->addr, addr, 6);
        f->cod = cod;
        /* a device that cannot be a pad (not a peripheral) needs no name */
        f->named = (cod != 0 && ((cod >> 8) & 0x1f) != 5);
        classify(f);
        bt_addr_text(addr, text);
        bt_log(st, "inquiry: %s cod=0x%06x rssi=%d%s", text, cod, rssi, f->gamepad ? " (game pad)" : "");
    }
    f->page_scan_mode = pscan;
    f->clock_offset = clock;
    f->rssi = rssi;
}

/* the next found device whose name is wanted, or -1 */
static int next_unnamed(struct bt_stack *st)
{
    int i;
    for (i = 0; i < st->found_count; i++)
        if (!st->found[i].named && st->found[i].name_tries < 3) return i;
    return -1;
}

static void request_name(struct bt_stack *st, int index)
{
    struct bt_found *f = &st->found[index];
    unsigned char p[10];
    memcpy(p, f->addr, 6);
    p[6] = (unsigned char)f->page_scan_mode;
    p[7] = 0;
    bt_put16(p + 8, 0x8000 | (f->clock_offset & 0x7fff));
    f->name_tries++;
    st->name_pending = index;
    st->name_started = bt_now();
    bt_hci_command(st, 0x0419, p, 10);
}

static void start_inquiry(struct bt_stack *st)
{
    unsigned char p[5];
    p[0] = 0x33; p[1] = 0x8b; p[2] = 0x9e;   /* the general inquiry access code */
    p[3] = SEARCH_INQUIRY_LENGTH;
    p[4] = 0;                                /* no limit on responses */
    st->inquiry_active = 1;
    bt_hci_command(st, 0x0401, p, 5);
}

static void cancel_inquiry(struct bt_stack *st)
{
    if (st->inquiry_active && !st->inquiry_cancelling) {
        st->inquiry_cancelling = 1;
        bt_hci_command(st, 0x0402, NULL, 0);
    }
}

/* ---- links ---- */

static void create_connection(struct bt_stack *st, const unsigned char *addr)
{
    unsigned char p[13];
    struct bt_found *f = bt_hci_found_by_addr(st, addr);
    char text[20];
    memcpy(p, addr, 6);
    p[6] = 0x18; p[7] = 0xcc;                /* packet types: DM1 DH1 DM3 DH3 DM5 DH5 */
    p[8] = (unsigned char)(f ? f->page_scan_mode : 1);
    p[9] = 0;
    if (f) bt_put16(p + 10, 0x8000 | (f->clock_offset & 0x7fff));
    else bt_put16(p + 10, 0);
    p[12] = 1;                               /* a role switch is allowed */
    bt_addr_text(addr, text);
    bt_log(st, "HCI: Create_Connection to %s", text);
    st->connecting = 1;
    st->connect_started = bt_now();
    st->link_incoming = 0;
    bt_hci_command(st, 0x0405, p, 13);
}

static void link_identity(struct bt_stack *st, const unsigned char *addr)
{
    struct bt_paired *k = bt_ssp_find_key(st, addr);
    struct bt_found *f = bt_hci_found_by_addr(st, addr);
    st->link_kind = BT_KIND_UNKNOWN;
    st->link_name[0] = 0;
    if (f && f->named) {
        st->link_kind = f->kind;
        strncpy(st->link_name, f->name, sizeof(st->link_name) - 1);
    }
    if (k && !st->link_name[0]) {
        st->link_kind = k->kind;
        strncpy(st->link_name, k->name, sizeof(st->link_name) - 1);
    }
}

void bt_hci_link_down(struct bt_stack *st)
{
    st->link_up = 0;
    st->link_authed = st->link_encrypted = 0;
    st->link_handle = 0xffff;
    st->link_used_stored_key = 0;
    st->auth_fallback_time = 0;
    st->l2cap_time = 0;
    st->pair_key_saved = 0;
    bt_l2cap_reset(st);
    bt_hid_reset(st);
}

static void connection_complete(struct bt_stack *st, const unsigned char *e, unsigned size)
{
    char text[20];
    if (size < 13) return;
    bt_addr_text(e + 5, text);
    bt_log(st, "HCI: Connection_Complete status 0x%02x handle 0x%03x %s", e[2], bt_get16(e + 3), text);
    st->connecting = 0;
    if (e[2] == 0) {
        st->link_handle = bt_get16(e + 3) & 0x0fff;
        memcpy(st->link_addr, e + 5, 6);
        st->link_up = 1;
        link_identity(st, e + 5);
        if (!st->link_incoming) {
            unsigned char p[2];
            bt_put16(p, st->link_handle);
            bt_log(st, "HCI: Authentication_Requested");
            bt_hci_command(st, 0x0411, p, 2);
        } else
            st->auth_fallback_time = bt_now() + 2.0;
        if (st->pairing) bt_say(st, "Pairing with %s...", st->link_name[0] ? st->link_name : text);
        else bt_say(st, "%s is connecting...", st->link_name[0] ? st->link_name : text);
    } else if (st->pairing && !memcmp(e + 5, st->pair_addr, 6)) {
        st->pair_tries++;
        if (st->pair_tries >= 3) {
            st->pairing = 0;
            bt_say(st, "Could not connect to the controller (status 0x%02x). Put it in pairing mode and try again.", e[2]);
        }
    }
}

static void connection_request(struct bt_stack *st, const unsigned char *e, unsigned size)
{
    unsigned char p[7];
    char text[20];
    int allowed;
    if (size < 13) return;
    bt_addr_text(e + 2, text);
    allowed = !st->link_up && !st->connecting &&
        (bt_hci_is_paired(st, e + 2) || (st->pairing && !memcmp(e + 2, st->pair_addr, 6)));
    bt_log(st, "HCI: Connection_Request from %s cod=0x%06x: %s", text, bt_get24(e + 8), allowed ? "accepted" : "refused");
    memcpy(p, e + 2, 6);
    if (allowed) {
        p[6] = 0;                            /* we become the central */
        st->link_incoming = 1;
        bt_hci_command(st, 0x0409, p, 7);
    } else {
        p[6] = 0x0f;                         /* rejected: unacceptable address */
        bt_hci_command(st, 0x040a, p, 7);
    }
}

void bt_hci_event(struct bt_stack *st, const unsigned char *e, unsigned size)
{
    unsigned i, n;
    if (size < 2) return;
    if (bt_ssp_event(st, e, size)) return;
    switch (e[0]) {
    case 0x01: /* Inquiry_Complete */
        st->inquiry_active = 0;
        st->inquiry_cancelling = 0;
        st->inquiry_ended = bt_now();
        break;
    case 0x02: /* Inquiry_Result: arrays by field: addresses, page scan mode, 2 reserved, class, clock offset */
        n = e[2];
        if (size < 3 + 14 * n) break;
        for (i = 0; i < n; i++) {
            const unsigned char *addr = e + 3 + 6 * i;
            unsigned pscan = e[3 + 6 * n + i];
            unsigned cod = bt_get24(e + 3 + 6 * n + 3 * n + 3 * i);
            unsigned clock = bt_get16(e + 3 + 6 * n + 3 * n + 3 * n + 2 * i);
            note_inquiry(st, addr, pscan, cod, clock, 0);
        }
        break;
    case 0x22: /* Inquiry_Result_with_RSSI: addresses, page scan mode, 1 reserved, class, clock offset, rssi */
        n = e[2];
        if (size < 3 + 14 * n) break;
        for (i = 0; i < n; i++) {
            const unsigned char *addr = e + 3 + 6 * i;
            unsigned pscan = e[3 + 6 * n + i];
            unsigned cod = bt_get24(e + 3 + 6 * n + 2 * n + 3 * i);
            unsigned clock = bt_get16(e + 3 + 6 * n + 2 * n + 3 * n + 2 * i);
            int rssi = (signed char)e[3 + 6 * n + 2 * n + 3 * n + 2 * n + i];
            note_inquiry(st, addr, pscan, cod, clock, rssi);
        }
        break;
    case 0x2f: /* Extended_Inquiry_Result (one response) */
        if (size >= 17) {
            const unsigned char *p = e + 3;
            note_inquiry(st, p, p[6], bt_get24(e + 3 + 8), bt_get16(e + 3 + 11), (signed char)p[13]);
        }
        break;
    case 0x07: /* Remote_Name_Request_Complete: status, address, name (248) */
        if (size >= 9) {
            struct bt_found *f = bt_hci_found_by_addr(st, e + 3);
            if (f) {
                if (e[2] == 0) {
                    unsigned len = size - 9 > BT_NAME_LENGTH - 1 ? BT_NAME_LENGTH - 1 : size - 9;
                    unsigned k;
                    memcpy(f->name, e + 9, len);
                    f->name[len] = 0;
                    for (k = 0; k < len; k++)
                        if (f->name[k] == 0) break;
                    f->name[k] = 0;
                    f->named = 1;
                    classify(f);
                    bt_log(st, "name: \"%s\"%s", f->name, f->supported ? "" : " (not a pad the stack reads)");
                } else if (f->name_tries >= 3)
                    f->named = 1;
            }
            if (st->name_pending >= 0 && f == &st->found[st->name_pending]) st->name_pending = -1;
        }
        break;
    case 0x03:
        connection_complete(st, e, size);
        break;
    case 0x04:
        connection_request(st, e, size);
        break;
    case 0x05: /* Disconnection_Complete: status, handle, reason */
        if (size >= 6) {
            bt_log(st, "HCI: Disconnection_Complete status 0x%02x handle 0x%03x reason 0x%02x",
                e[2], bt_get16(e + 3), e[5]);
            if (e[2] == 0 && st->link_up && (bt_get16(e + 3) & 0x0fff) == st->link_handle) {
                bt_hci_link_down(st);
                if (!st->pairing) bt_say(st, "Controller disconnected. Press PS on it to connect again.");
            }
        }
        break;
    case 0x13: /* Number_Of_Completed_Packets */
        n = e[2];
        for (i = 0; i < n && 3 + i * 4 + 4 <= size; i++) {
            st->acl_credits += (int)bt_get16(e + 3 + i * 4 + 2);
            if (st->acl_credits > 64) st->acl_credits = 64;
        }
        break;
    case 0x0e: /* Command_Complete: credits, opcode, status, return parameters */
        if (size >= 6) {
            unsigned opcode = bt_get16(e + 3);
            st->ncmd = e[2];
            if (opcode == 0x0402) {
                /* Inquiry_Cancel completes with this event alone: no Inquiry_Complete follows */
                st->inquiry_active = 0;
                st->inquiry_cancelling = 0;
                st->inquiry_ended = bt_now();
            }
            if (opcode == st->sync_opcode && st->sync_wait) {
                st->sync_status = e[5];
                st->sync_length = size - 5 > sizeof(st->sync_return) ? (unsigned)sizeof(st->sync_return) : size - 5;
                memcpy(st->sync_return, e + 5, st->sync_length);
                st->sync_done = 1;
            } else if (e[5] != 0)
                bt_log(st, "HCI: command 0x%04x complete with status 0x%02x", opcode, e[5]);
        }
        break;
    case 0x0f: /* Command_Status: status, credits, opcode */
        if (size >= 6) {
            unsigned opcode = bt_get16(e + 4);
            st->ncmd = e[3];
            if (e[2] != 0) {
                bt_log(st, "HCI: command 0x%04x status 0x%02x", opcode, e[2]);
                if (opcode == 0x0405) st->connecting = 0;
                if (opcode == 0x0419) st->name_pending = -1;
                if (opcode == 0x0401) st->inquiry_active = 0;
            }
        }
        break;
    default:
        break;  /* (mode, role, page-scan repetition, max slots and the like: nothing to do) */
    }
}

/* ---- the controller's start-up ---- */

int bt_stack_start(struct bt_stack *st)
{
    unsigned char r[300];
    unsigned rs = 0;
    int status;
    static const unsigned char event_mask[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f };
    unsigned char name[248];

    status = bt_hci_sync(st, 0x0c03, NULL, 0, r, &rs, 5.0);
    if (status != 0) { bt_say(st, "The dongle does not answer (Reset: %d)", status); return -1; }
    status = bt_hci_sync(st, 0x1001, NULL, 0, r, &rs, 3.0);
    if (status != 0 || rs < 9) { bt_say(st, "The dongle does not tell its version (%d)", status); return -1; }
    st->hci_version = r[1];
    st->hci_revision = bt_get16(r + 2);
    st->lmp_subversion = bt_get16(r + 7);
    bt_log(st, "version: hci_version 0x%02x hci_revision 0x%04x lmp_subversion 0x%04x", st->hci_version,
        st->hci_revision, st->lmp_subversion);
    if (st->lmp_subversion == 0x8761) {
        /* the Realtek ROM: it needs its patch */
        status = bt_hci_sync(st, 0xfc6d, NULL, 0, r, &rs, 3.0);
        st->rom_version = (status == 0 && rs >= 2) ? r[1] : 0;
        bt_log(st, "Realtek ROM version %d", st->rom_version);
        if (!st->firmware_path[0]) { bt_say(st, "The dongle needs its Realtek firmware (rtl8761bu_fw.bin) and none was found."); return -1; }
        bt_say(st, "Loading the firmware...");
        if (bt_realtek_load(st, (unsigned)st->rom_version) < 0) return -1;
        status = bt_hci_sync(st, 0x0c03, NULL, 0, r, &rs, 5.0);
        if (status != 0) { bt_say(st, "The dongle does not answer after the firmware (%d)", status); return -1; }
        status = bt_hci_sync(st, 0x1001, NULL, 0, r, &rs, 3.0);
        if (status == 0 && rs >= 9) {
            st->hci_revision = bt_get16(r + 2);
            st->lmp_subversion = bt_get16(r + 7);
        }
        st->firmware_loaded = 1;
    } else
        st->firmware_loaded = 1;
    sprintf(st->chip, "Realtek RTL8761B, firmware %04x%04x", st->hci_revision, st->lmp_subversion);
    status = bt_hci_sync(st, 0x1005, NULL, 0, r, &rs, 3.0);
    if (status == 0 && rs >= 8) {
        st->acl_max = bt_get16(r + 1);
        st->acl_credits = (int)bt_get16(r + 4);
        if (st->acl_max > 1000) st->acl_max = 1000;
        if (st->acl_max < 27) st->acl_max = 27;
        bt_log(st, "buffers: ACL max %u, %d packets", st->acl_max, st->acl_credits);
    }
    status = bt_hci_sync(st, 0x1009, NULL, 0, r, &rs, 3.0);
    if (status != 0 || rs < 7) { bt_say(st, "The dongle does not tell its address"); return -1; }
    memcpy(st->local_addr, r + 1, 6);
    {
        char text[20];
        bt_addr_text(st->local_addr, text);
        bt_log(st, "local address %s", text);
    }
    if (st->keys_path[0]) st->paired_count = bt_keys_load(st->keys_path, st->local_addr, st->paired, BT_MAX_PAIRED);
    bt_log(st, "%d stored link key(s)", st->paired_count);
    bt_hci_sync(st, 0x0c01, event_mask, 8, r, &rs, 3.0);
    memset(name, 0, sizeof(name));
    strcpy((char *)name, "PowerSpark");
    bt_hci_sync(st, 0x0c13, name, 248, r, &rs, 3.0);
    bt_hci_sync(st, 0x0c24, (const unsigned char *)"\x0c\x01\x00", 3, r, &rs, 3.0);   /* class: computer, laptop */
    bt_hci_sync(st, 0x0c45, (const unsigned char *)"\x01", 1, r, &rs, 3.0);           /* inquiry results with RSSI */
    status = bt_hci_sync(st, 0x0c56, (const unsigned char *)"\x01", 1, r, &rs, 3.0); /* Secure Simple Pairing on */
    if (status != 0) { bt_say(st, "Secure Simple Pairing is not available (status %d)", status); return -1; }
    status = bt_hci_sync(st, 0x0c1a, (const unsigned char *)"\x02", 1, r, &rs, 3.0); /* page scan: a pad can connect to us */
    st->reconnect = status == 0;
    st->dongle = BT_DONGLE_READY;
    if (st->paired_count)
        bt_say(st, "Ready. Press PS on a paired controller to connect it, or search for a new one.");
    else
        bt_say(st, "Ready. Put a controller in pairing mode and search for it.");
    return 0;
}

void bt_stack_stop(struct bt_stack *st)
{
    unsigned char p[3];
    if (st->tr.send_command && !st->failed) {
        if (st->link_up) {
            bt_l2cap_disconnect_all(st);
            bt_stack_pump(st, 0.3);
            bt_put16(p, st->link_handle);
            p[2] = 0x13;
            bt_hci_command(st, 0x0406, p, 3);
            bt_stack_pump(st, 0.8);
        }
        if (st->inquiry_active) { cancel_inquiry(st); bt_stack_pump(st, 0.3); }
        p[0] = 0;
        bt_hci_command(st, 0x0c1a, p, 1);   /* not connectable any more */
        bt_stack_pump(st, 0.2);
    }
    if (st->tr.close) st->tr.close(st->tr.ctx);
    st->tr.send_command = NULL;
}

/* ---- requests ---- */

void bt_stack_search(struct bt_stack *st, double seconds)
{
    int i;
    if (st->link_up) { bt_say(st, "Disconnect the controller before searching."); return; }
    /* names are asked again for what has not answered; devices gone are forgotten */
    for (i = 0; i < st->found_count; i++) st->found[i].name_tries = 0;
    st->found_count = 0;
    st->name_pending = -1;
    st->search_until = bt_now() + seconds;
    bt_say(st, "Searching... put the controller in pairing mode (DualSense: hold Create and PS until the light flashes fast).");
}

void bt_stack_search_stop(struct bt_stack *st)
{
    st->search_until = 0;
    cancel_inquiry(st);
}

void bt_stack_pair(struct bt_stack *st, const unsigned char *addr)
{
    struct bt_found *f = bt_hci_found_by_addr(st, addr);
    if (st->link_up) { bt_say(st, "Disconnect the controller first."); return; }
    memcpy(st->pair_addr, addr, 6);
    st->pairing = 1;
    st->pair_tries = 0;
    st->pair_key_saved = 0;
    st->pair_deadline = bt_now() + PAIR_TIMEOUT;
    st->search_until = 0;
    st->connect_started = 0;
    cancel_inquiry(st);
    bt_say(st, "Pairing with %s...", f && f->name[0] ? f->name : "the controller");
}

void bt_stack_forget(struct bt_stack *st, const unsigned char *addr)
{
    int i;
    for (i = 0; i < st->paired_count; i++)
        if (!memcmp(st->paired[i].addr, addr, 6)) {
            int k;
            for (k = i; k + 1 < st->paired_count; k++) st->paired[k] = st->paired[k + 1];
            st->paired_count--;
            break;
        }
    if (st->keys_path[0]) bt_keys_remove(st->keys_path, st->local_addr, addr);
    if (st->link_up && !memcmp(st->link_addr, addr, 6)) bt_stack_disconnect(st);
    bt_say(st, "Forgot the controller. Pair it again to use it.");
}

void bt_stack_disconnect(struct bt_stack *st)
{
    if (st->link_up) {
        unsigned char p[3];
        bt_l2cap_disconnect_all(st);
        bt_put16(p, st->link_handle);
        p[2] = 0x13;
        bt_hci_command(st, 0x0406, p, 3);
    }
}

void bt_stack_output(struct bt_stack *st, const struct bt_pad_output *o)
{
    if (o->set_light) {
        st->output.red = o->red; st->output.green = o->green; st->output.blue = o->blue;
        st->output.set_light = 1;
    }
    if (o->set_players) {
        st->output.player_leds = o->player_leds;
        st->output.set_players = 1;
    }
    if (o->set_rumble) {
        st->output.strong = o->strong;
        st->output.weak = o->weak;
        st->output.set_rumble = 1;
        st->rumble_off_time = (o->strong || o->weak) ? bt_now() + 0.4 : 0;
    }
    st->output_dirty = 1;
    st->output_time = 0;
}

/* ---- what is due ---- */

void bt_hci_tick(struct bt_stack *st)
{
    double t = bt_now();

    if (st->name_pending >= 0 && t - st->name_started > NAME_TIMEOUT) st->name_pending = -1;

    if (st->search_until) {
        if (t >= st->search_until || st->link_up || st->pairing) {
            st->search_until = 0;
            cancel_inquiry(st);
            if (!st->pairing && !st->link_up)
                bt_say(st, st->found_count ? "Search finished." : "Search finished: nothing found. Is the controller in pairing mode?");
        } else if (!st->inquiry_active && !st->inquiry_cancelling && st->name_pending < 0) {
            int next = next_unnamed(st);
            if (next >= 0) request_name(st, next);
            else if (t - st->inquiry_ended > 0.3) start_inquiry(st);
        }
    }

    if (st->pairing) {
        if (t > st->pair_deadline) {
            st->pairing = 0;
            bt_say(st, "Pairing timed out. Put the controller in pairing mode and try again.");
        } else if (!st->link_up && !st->connecting && !st->inquiry_active && !st->inquiry_cancelling &&
                st->name_pending < 0 && t - st->connect_started > 1.0)
            create_connection(st, st->pair_addr);
    } else if (st->reconnect && !st->link_up && !st->connecting && !st->inquiry_active && !st->inquiry_cancelling &&
            !st->search_until && st->name_pending < 0 && st->paired_count > 0 && t - st->page_time > PAGE_INTERVAL) {
        /* a pad that has just been woken answers a page as well as it pages us */
        st->page_time = t;
        create_connection(st, st->paired[(int)((unsigned long)(t * 10) % (unsigned long)st->paired_count)].addr);
    }

    if (st->link_up && !st->link_authed && st->auth_fallback_time && t >= st->auth_fallback_time) {
        unsigned char p[2];
        st->auth_fallback_time = 0;
        bt_put16(p, st->link_handle);
        bt_log(st, "no authentication from the pad; requesting it");
        bt_hci_command(st, 0x0411, p, 2);
    }
    if (st->link_up && st->l2cap_time && t >= st->l2cap_time) {
        st->l2cap_time = 0;
        if (!bt_l2cap_by_psm(st, 0x11)) bt_l2cap_connect(st, 0x11);
    }
    if (st->pairing && st->hid_ready) {
        st->pairing = 0;
        bt_say(st, "Paired with %s.", st->link_name[0] ? st->link_name : "the controller");
    }
    bt_hid_tick(st);
}

void bt_stack_tick(struct bt_stack *st)
{
    bt_hci_tick(st);
}
