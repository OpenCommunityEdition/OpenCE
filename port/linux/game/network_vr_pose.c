/* Optional visual avatar extension. Never writes simulation palettes, hitboxes,
 * movement or weapons. The host offers support; old-host clients send nothing.
 * Only acknowledged peers receive poses. Every client pose is owner checked. */
#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "units/units.h"
#include "objects/objects.h"
#include "networking/network_game_globals.h"
#include "network_distributed.h"
#include "network_vr_pose.h"
#include <math.h>
#include <string.h>

unsigned long system_milliseconds(void);
void platform_log(char const *format, ...);
enum { VR_POSE_MAGIC = 0x56525031, VR_POSE_NODES = 64, VR_POSE_LEASE = 6000 };
struct vr_capability { unsigned long magic, seed, nonce; long round; word version, kind; };
struct vr_joint {
    unsigned long magic, seed, sequence;
    long round, unit;
    byte player, node, count, reserved;
    real scale;
    real_point3d position;
    real_vector3d forward, up;
};
typedef char vr_joint_wire_size[sizeof(struct vr_joint) == 64 ? 1 : -1];
typedef char vr_capability_wire_size[sizeof(struct vr_capability) == 20 ? 1 : -1];
static struct { unsigned long nonce, seen; long player; boolean enabled; } peers[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
static boolean host_enabled;
static unsigned long host_nonce, host_seen, last_offer, next_nonce = 1;
static long logged_local_unit = NONE;
/* test21: when a client joined, and whether it said its host never offered
 * avatars (a host on another build: everyone then sees stock animation) */
static unsigned long client_since;
static boolean client_unsupported_logged;
static struct vr_pose_state {
    real_matrix4x3 pending[VR_POSE_NODES], previous[VR_POSE_NODES], current[VR_POSE_NODES];
    unsigned long sequence, pending_sequence, received, rendered_at, last_sent;
    unsigned long long mask;
    long unit, pending_unit;
    short count, pending_count;
    boolean valid, assembling;
} poses[MAXIMUM_TRACKED_PLAYERS];
static struct { real_matrix4x3 nodes[VR_POSE_NODES]; long unit;
    short count; unsigned long sequence, captured; boolean valid; } local_pose;

void network_vr_pose_reset(void)
{
    memset(peers, 0, sizeof(peers)); memset(poses, 0, sizeof(poses));
    memset(&local_pose, 0, sizeof(local_pose));
    host_enabled = FALSE; host_nonce = host_seen = last_offer = 0;
    logged_local_unit = NONE;
    client_since = 0; client_unsupported_logged = FALSE;
}

boolean network_vr_pose_wanted(void)
{
    unsigned long now = system_milliseconds();
    if (game_connection() == _game_connection_network_client)
        return host_enabled && now - host_seen <= VR_POSE_LEASE;
    if (game_connection() == _game_connection_network_server)
        for (short m = 0; m < NUMBEROF(peers); m++)
            if (peers[m].enabled && now - peers[m].seen <= VR_POSE_LEASE) return TRUE;
    return FALSE;
}

static boolean active_unit(long unit, byte player, short count)
{
    struct player_datum *p = distributed_player(player);
    struct unit_datum *u;
    if (!p || count <= 0 || count > VR_POSE_NODES || p->unit_index != unit ||
        !distributed_object_index_valid(unit) || !(u = unit_try_and_get(unit))) return FALSE;
    return u->object.type == _object_type_biped && u->object.parent_object_index == NONE &&
        !TEST_FLAG(u->object.damage_flags, _object_dead_bit) &&
        u->object.node_matrices.size == count * sizeof(real_matrix4x3);
}

static void capability(long machine, word kind, unsigned long nonce)
{
    struct { struct distributed_message_header header; struct vr_capability body; } m;
    memset(&m, 0, sizeof(m));
    m.body.magic = VR_POSE_MAGIC; m.body.version = 1; m.body.kind = kind; m.body.nonce = nonce;
    m.body.round = network_game_get_number_of_games_played(); m.body.seed = network_game_get_random_seed();
    if (machine == NONE) distributed_send(&m, _distributed_message_vr_capability, 1, sizeof(m), _distributed_to_host);
    else distributed_send_to_machine(machine, &m, _distributed_message_vr_capability, 1, sizeof(m));
}

static long machine_player(long machine)
{
    for (short p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++)
        if (distributed_player(p) && distributed_machine_has_player(machine, p))
            return distributed_player_from_byte((byte)p);
    return NONE;
}

void network_vr_pose_capture(long unit, real_matrix4x3 const *m, short count)
{
    struct unit_datum *u = unit_try_and_get(unit);
    byte player = u ? distributed_player_to_byte(u->unit.player_index) : NO_PLAYER;
    if (!u || !distributed_player_is_local(u->unit.player_index) || !active_unit(unit, player, count)) return;
    for (short n = 0; n < count; n++) {
        local_pose.nodes[n] = m[n];
        local_pose.nodes[n].position.x -= u->object.position.x;
        local_pose.nodes[n].position.y -= u->object.position.y;
        local_pose.nodes[n].position.z -= u->object.position.z;
    }
    local_pose.unit = unit; local_pose.count = count;
    local_pose.sequence++; local_pose.captured = system_milliseconds(); local_pose.valid = TRUE;
}

static void send_pose(long machine, byte player, long unit, unsigned long sequence,
    real_matrix4x3 const *m, short count)
{
    struct { struct distributed_message_header header;
        struct vr_joint joints[DATAGRAM_ENTRIES(struct vr_joint)]; } message;
    short used = 0;
    for (short n = 0; n < count; n++) {
        struct vr_joint *j = &message.joints[used++];
        memset(j, 0, sizeof(*j)); j->magic = VR_POSE_MAGIC;
        j->round = network_game_get_number_of_games_played(); j->seed = network_game_get_random_seed();
        j->sequence = sequence; j->unit = unit; j->player = player; j->node = n; j->count = count;
        j->scale = m[n].scale; j->position = m[n].position; j->forward = m[n].forward; j->up = m[n].up;
        if (used == NUMBEROF(message.joints) || n == count - 1) {
            word bytes = sizeof(message.header) + used * sizeof(*j);
            if (machine == NONE) distributed_send(&message, _distributed_message_vr_pose, used, bytes, _distributed_to_host);
            else distributed_send_to_machine(machine, &message, _distributed_message_vr_pose, used, bytes);
            used = 0;
        }
    }
}

void network_vr_pose_tick(void)
{
    unsigned long now = system_milliseconds();
    boolean server = game_connection() == _game_connection_network_server;
    long machines[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
    short count = server ? distributed_client_machines(machines, NUMBEROF(machines)) : 0;
    if (server && (!last_offer || now - last_offer >= 2000)) {
        boolean present[HALO_PORT_MAXIMUM_NETWORK_MACHINES] = {FALSE};
        last_offer = now;
        for (short n = 0; n < count; n++) {
            long machine = machines[n], player = machine_player(machine);
            if (!VALID_INDEX(machine, HALO_PORT_MAXIMUM_NETWORK_MACHINES) || player == NONE) continue;
            present[machine] = TRUE;
            if (!peers[machine].nonce || peers[machine].player != player) {
                peers[machine].enabled = FALSE; peers[machine].player = player;
                peers[machine].nonce = ++next_nonce;
                if (!peers[machine].nonce) peers[machine].nonce = ++next_nonce;
            }
            capability(machine, 1, peers[machine].nonce);
        }
        for (short absent = 0; absent < NUMBEROF(peers); absent++)
            if (!present[absent]) memset(&peers[absent], 0, sizeof(peers[absent]));
    }
    if (game_connection() == _game_connection_network_client) {
        if (!client_since) client_since = now ? now : 1;
        if (!host_nonce && !client_unsupported_logged && now - client_since > 10000) {
            client_unsupported_logged = TRUE;
            platform_log("vr pose: this host offered no VR avatars in 10 s (it runs another build): players here see stock "
                "animation. Host the game from this build for VR movement to be shared");
        }
    }
    if (game_time_get() % 2) return; /* 15 Hz visual snapshots; simulation remains 30 Hz. */
    for (short n = 0; n < (server ? count : 1); n++) {
        long machine = server ? machines[n] : NONE;
        if (server) {
            if (!VALID_INDEX(machine, HALO_PORT_MAXIMUM_NETWORK_MACHINES) || !peers[machine].enabled ||
                now - peers[machine].seen > VR_POSE_LEASE || peers[machine].player != machine_player(machine)) continue;
        } else if (!host_enabled || now - host_seen > VR_POSE_LEASE) continue;
        if (local_pose.valid && now - local_pose.captured < 250) {
            struct unit_datum *u = unit_try_and_get(local_pose.unit);
            byte p = u ? distributed_player_to_byte(u->unit.player_index) : NO_PLAYER;
            if (active_unit(local_pose.unit, p, local_pose.count) &&
                (!server || distributed_machine_sees_player(machine, p))) {
                if (logged_local_unit != local_pose.unit) {
                    logged_local_unit = local_pose.unit;
                    platform_log("vr pose: publishing unit %ld, %d nodes at 15 Hz", local_pose.unit, local_pose.count);
                }
                send_pose(machine, p, local_pose.unit, local_pose.sequence, local_pose.nodes, local_pose.count);
            }
        }
        if (server) for (short p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++) {
            struct vr_pose_state *pose = &poses[p];
            if (pose->valid && now - pose->received < 250 &&
                !distributed_machine_has_player(machine, p) && distributed_machine_sees_player(machine, p) &&
                active_unit(pose->unit, p, pose->count))
                send_pose(machine, p, pose->unit, pose->sequence, pose->current, pose->count);
        }
    }
}

void network_vr_pose_receive(long machine, byte type, void const *data, short count, word size)
{
    unsigned long now = system_milliseconds();
    boolean server = game_connection() == _game_connection_network_server;
    if ((server && !VALID_INDEX(machine, HALO_PORT_MAXIMUM_NETWORK_MACHINES)) ||
        (!server && (game_connection() != _game_connection_network_client || machine != NONE))) return;
    if (type == _distributed_message_vr_capability) {
        struct vr_capability c;
        if (count != 1 || size != sizeof(c)) return;
        memcpy(&c, data, sizeof(c));
        if (c.magic != VR_POSE_MAGIC || c.version != 1 || !c.nonce ||
            c.round != network_game_get_number_of_games_played() || c.seed != (unsigned long)network_game_get_random_seed()) return;
        if (server && c.kind == 2 && peers[machine].nonce == c.nonce && peers[machine].player == machine_player(machine)) {
            if (!peers[machine].enabled) platform_log("vr pose: peer %ld negotiated visual avatars v1", machine);
            peers[machine].enabled = TRUE; peers[machine].seen = now; capability(machine, 3, c.nonce);
        } else if (!server && c.kind == 1) {
            if (host_nonce != c.nonce) host_enabled = FALSE;
            host_nonce = c.nonce; capability(NONE, 2, c.nonce);
        } else if (!server && c.kind == 3 && host_nonce == c.nonce) {
            if (!host_enabled) platform_log("vr pose: host negotiated visual avatars v1");
            host_enabled = TRUE; host_seen = now;
        }
        return;
    }
    if (type != _distributed_message_vr_pose || count <= 0 || size != count * sizeof(struct vr_joint)) return;
    if (server ? (!peers[machine].enabled || now - peers[machine].seen > VR_POSE_LEASE ||
        peers[machine].player != machine_player(machine)) : (!host_enabled || now - host_seen > VR_POSE_LEASE)) return;
    for (short n = 0; n < count; n++) {
        struct vr_joint j; real_matrix4x3 m; struct vr_pose_state *p;
        memcpy(&j, (byte const *)data + n * sizeof(j), sizeof(j));
        if (j.magic != VR_POSE_MAGIC || j.reserved || j.round != network_game_get_number_of_games_played() ||
            j.seed != (unsigned long)network_game_get_random_seed() || !VALID_INDEX(j.player, MAXIMUM_TRACKED_PLAYERS) ||
            j.node >= j.count || !active_unit(j.unit, j.player, j.count) ||
            distributed_player_is_local(distributed_player_from_byte(j.player)) ||
            (server && !distributed_machine_has_player(machine, j.player)) ||
            !(j.scale >= 0.25f && j.scale <= 4.0f) || !distributed_point_valid(&j.position, 4.0f) ||
            !distributed_transform_valid(&j.position, &j.forward, &j.up, NULL, NULL, &m.forward, &m.up)) continue;
        cross_product3d(&m.up, &m.forward, &m.left); m.scale = j.scale; m.position = j.position;
        p = &poses[j.player];
        if (p->valid && p->unit == j.unit && (long)(j.sequence - p->sequence) <= 0) continue;
        if (!p->assembling || p->pending_unit != j.unit || p->pending_sequence != j.sequence) {
            if (p->assembling && p->pending_unit == j.unit && (long)(j.sequence - p->pending_sequence) < 0) continue;
            p->mask = 0; p->pending_sequence = j.sequence; p->pending_unit = j.unit; p->pending_count = j.count; p->assembling = TRUE;
        }
        if (p->pending_count != j.count) continue;
        p->pending[j.node] = m; p->mask |= 1ULL << j.node;
        if (p->mask == (j.count == 64 ? ~0ULL : (1ULL << j.count) - 1)) {
            boolean continuous = p->valid && p->unit == j.unit && p->count == j.count && now - p->received < 250;
            if (!p->valid || p->unit != j.unit)
                platform_log("vr pose: received complete avatar for player %d unit %ld, %d nodes", j.player, j.unit, j.count);
            memcpy(p->previous, continuous ? p->current : p->pending, j.count * sizeof(m));
            memcpy(p->current, p->pending, j.count * sizeof(m));
            p->unit = j.unit; p->count = j.count; p->sequence = j.sequence;
            p->received = now; p->valid = TRUE; p->assembling = FALSE;
        }
    }
}

static real_matrix4x3 *render_unit_pose(long unit, real_matrix4x3 *stock)
{
    static real_matrix4x3 result[VR_POSE_NODES];
    struct unit_datum *u; struct vr_pose_state *p; byte player;
    unsigned long now = system_milliseconds(), age; real blend;
    if (!stock || (game_connection() != _game_connection_network_server && game_connection() != _game_connection_network_client) ||
        !(u = unit_try_and_get(unit))) return stock;
    player = distributed_player_to_byte(u->unit.player_index);
    if (!VALID_INDEX(player, MAXIMUM_TRACKED_PLAYERS) || distributed_player_is_local(u->unit.player_index)) return stock;
    p = &poses[player]; age = now - p->received;
    if (!p->valid || p->unit != unit || age > 500 || !active_unit(unit, player, p->count)) return stock;
    blend = MIN(age / 67.0f, 1.0f);
    for (short n = 0; n < p->count; n++) {
        real_quaternion a, b, q;
        matrix4x3_rotation_to_quaternion(&p->previous[n], &a);
        matrix4x3_rotation_to_quaternion(&p->current[n], &b);
        quaternions_interpolate_and_normalize(&a, &b, blend, &q);
        matrix4x3_rotation_from_quaternion(&result[n], &q);
        result[n].scale = p->previous[n].scale + (p->current[n].scale - p->previous[n].scale) * blend;
        for (short axis = 0; axis < 3; axis++)
            result[n].position.n[axis] = u->object.position.n[axis] + p->previous[n].position.n[axis] +
                (p->current[n].position.n[axis] - p->previous[n].position.n[axis]) * blend;
        if (age > 250) {
            real fade = (age - 250) / 250.0f;
            real scale = result[n].scale;
            real_point3d position = result[n].position;
            matrix4x3_rotation_to_quaternion(&result[n], &a);
            matrix4x3_rotation_to_quaternion(&stock[n], &b);
            quaternions_interpolate_and_normalize(&a, &b, fade, &q);
            matrix4x3_rotation_from_quaternion(&result[n], &q);
            result[n].scale = scale + (stock[n].scale - scale) * fade;
            for (short axis = 0; axis < 3; axis++)
                result[n].position.n[axis] = position.n[axis] + (stock[n].position.n[axis] - position.n[axis]) * fade;
        }
    }
    return result;
}

real_matrix4x3 *network_vr_pose_render(long unit, real_matrix4x3 *stock)
{
    static real_matrix4x3 attached[VR_POSE_NODES];
    struct object_datum *object;
    if (!stock || !distributed_object_index_valid(unit)) return stock;
    object = object_get(unit);
    if (object->object.type == _object_type_biped) return render_unit_pose(unit, stock);
    /* Carry a held weapon with the rendered hand, without moving its physics
     * or shot origin. The parent-node binding is the engine's own attachment. */
    if (object->object.type == _object_type_weapon && object->object.parent_object_index != NONE) {
        struct unit_datum *parent = unit_try_and_get(object->object.parent_object_index);
        short node = object->object.parent_node_index;
        short count = object->object.node_matrices.size / sizeof(real_matrix4x3);
        if (parent && parent->object.type == _object_type_biped && node >= 0 &&
            node < (short)(parent->object.node_matrices.size / sizeof(real_matrix4x3)) && count > 0 && count <= VR_POSE_NODES) {
            real_matrix4x3 *base = object_get_node_matrices(object->object.parent_object_index);
            real_matrix4x3 *pose = render_unit_pose(object->object.parent_object_index, base);
            if (base && pose != base) {
                real_matrix4x3 inverse, delta;
                matrix4x3_inverse(&base[node], &inverse);
                matrix4x3_multiply(&pose[node], &inverse, &delta);
                for (short n = 0; n < count; n++) matrix4x3_multiply(&delta, &stock[n], &attached[n]);
                return attached;
            }
        }
    }
    return stock;
}
