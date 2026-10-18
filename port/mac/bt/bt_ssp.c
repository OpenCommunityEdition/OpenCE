/* Authentication and Secure Simple Pairing (bt_stack.h). The stack declares
   "no input, no output" (just works) with general bonding and accepts the
   confirmation, which is what a game pad expects of a host: it has nothing to
   compare a number with. A pad that is already paired is authenticated with
   its stored link key; when pairing is wanted, the stored key is refused so
   that the pad and the stack agree on a new one. */
#include "bt_stack.h"

#include <string.h>

struct bt_paired *bt_ssp_find_key(struct bt_stack *st, const unsigned char *addr)
{
    int i;
    for (i = 0; i < st->paired_count; i++)
        if (!memcmp(st->paired[i].addr, addr, 6)) return &st->paired[i];
    return NULL;
}

static int is_link(const struct bt_stack *st, const unsigned char *addr)
{
    return st->link_up && !memcmp(addr, st->link_addr, 6);
}

static void save_key(struct bt_stack *st, const unsigned char *addr, const unsigned char *key, int type)
{
    struct bt_paired k;
    struct bt_paired *old = bt_ssp_find_key(st, addr);
    struct bt_found *f = bt_hci_found_by_addr(st, addr);
    memset(&k, 0, sizeof(k));
    memcpy(k.addr, addr, 6);
    memcpy(k.key, key, 16);
    k.key_type = type;
    k.kind = st->link_kind;
    strncpy(k.name, st->link_name, sizeof(k.name) - 1);
    if (!k.name[0] && f) strncpy(k.name, f->name, sizeof(k.name) - 1);
    if (!k.kind && f) k.kind = f->kind;
    if (old) *old = k;
    else if (st->paired_count < BT_MAX_PAIRED) st->paired[st->paired_count++] = k;
    if (st->keys_path[0]) {
        if (bt_keys_store(st->keys_path, st->local_addr, &k) != 0) bt_log(st, "cannot write %s", st->keys_path);
        else bt_log(st, "link key (type %d) saved to %s", type, st->keys_path);
    }
}

int bt_ssp_event(struct bt_stack *st, const unsigned char *e, unsigned size)
{
    unsigned char p[24];
    switch (e[0]) {
    case 0x17: /* Link_Key_Request */
        if (size < 8) return 1;
        memcpy(p, e + 2, 6);
        {
            struct bt_paired *k = bt_ssp_find_key(st, e + 2);
            if (k && !(st->pairing && !memcmp(e + 2, st->pair_addr, 6))) {
                memcpy(p + 6, k->key, 16);
                st->link_used_stored_key = 1;
                bt_hci_command(st, 0x040b, p, 22);
                bt_log(st, "HCI: Link_Key_Request: replied with the stored link key");
            } else {
                bt_hci_command(st, 0x040c, p, 6);
                bt_log(st, "HCI: Link_Key_Request: no key to give (pairing follows)");
            }
        }
        return 1;
    case 0x18: /* Link_Key_Notification: address, key (16), type */
        if (size >= 25 && is_link(st, e + 2)) {
            bt_log(st, "HCI: Link_Key_Notification, key type %u", e[24]);
            save_key(st, e + 2, e + 8, e[24]);
            st->pair_key_saved = 1;
        }
        return 1;
    case 0x31: /* IO_Capability_Request */
        if (size < 8) return 1;
        memcpy(p, e + 2, 6);
        p[6] = 0x03; /* NoInputNoOutput */
        p[7] = 0x00; /* no OOB data */
        p[8] = 0x04; /* general bonding, MITM protection not required */
        bt_log(st, "HCI: IO_Capability_Request: NoInputNoOutput, no OOB, general bonding");
        bt_hci_command(st, 0x042b, p, 9);
        return 1;
    case 0x32:
        if (size >= 11) bt_log(st, "HCI: IO_Capability_Response: capability %u oob %u auth 0x%02x", e[8], e[9], e[10]);
        return 1;
    case 0x33: /* User_Confirmation_Request */
        if (size < 8) return 1;
        memcpy(p, e + 2, 6);
        bt_log(st, "HCI: User_Confirmation_Request (just works): accepting");
        bt_hci_command(st, 0x042c, p, 6);
        return 1;
    case 0x34: /* User_Passkey_Request */
        if (size < 8) return 1;
        memcpy(p, e + 2, 6);
        bt_log(st, "HCI: User_Passkey_Request: refusing");
        bt_hci_command(st, 0x042e, p, 6);
        return 1;
    case 0x36: /* Simple_Pairing_Complete */
        bt_log(st, "HCI: Simple_Pairing_Complete status 0x%02x", size > 2 ? e[2] : 0xff);
        if (size > 2 && e[2] != 0 && st->pairing) {
            st->pairing = 0;
            bt_say(st, "Pairing failed (status 0x%02x). Put the controller in pairing mode again.", e[2]);
        }
        return 1;
    case 0x06: /* Authentication_Complete */
        if (size < 5) return 1;
        bt_log(st, "HCI: Authentication_Complete status 0x%02x", e[2]);
        if (e[2] == 0) {
            st->link_authed = 1;
            p[0] = (unsigned char)(st->link_handle & 0xff);
            p[1] = (unsigned char)(st->link_handle >> 8);
            p[2] = 1;
            bt_hci_command(st, 0x0413, p, 3); /* Set_Connection_Encryption */
        } else if (st->pairing) {
            st->pairing = 0;
            bt_say(st, "Pairing failed (authentication status 0x%02x). Put the controller in pairing mode again.", e[2]);
        } else if (st->link_used_stored_key) {
            bt_say(st, "The controller does not accept the stored key (status 0x%02x): forget it, then pair again.", e[2]);
        }
        return 1;
    case 0x08: /* Encryption_Change */
        if (size < 6) return 1;
        bt_log(st, "HCI: Encryption_Change status 0x%02x enabled %u", e[2], e[5]);
        if (e[2] == 0 && e[5]) {
            st->link_encrypted = 1;
            st->l2cap_time = bt_now() + (st->link_incoming ? 1.5 : 0.05);
        }
        return 1;
    default:
        return 0;
    }
}
