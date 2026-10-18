"""Takes what the network's wire is made of from the game's sources, for tools/net_wire_golden.c to compile
and run: the structures of the messages, their layouts and the functions that write and read them, unchanged.

    python3 tools/net_wire_extract.py build/net-wire/extracted.h
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GAME = ROOT / "port/linux/game"


def text(path):
    return (ROOT / path).read_text()


def struct(path, name):
    """struct name { ... }; as written (from its line to the closing brace's)"""
    source = text(path)
    match = re.search(r"^struct %s\n\{.*?^\};\n" % re.escape(name), source, re.S | re.M)
    if not match:
        raise SystemExit("%s: struct %s not found" % (path, name))
    return match.group(0)


def define(path, name):
    """#define NAME ... with its continuation lines"""
    lines = text(path).split("\n")
    for index, line in enumerate(lines):
        if re.match(r"#define %s\b" % re.escape(name), line):
            end = index
            while lines[end].endswith("\\"):
                end += 1
            return "\n".join(lines[index:end + 1]) + "\n"
    raise SystemExit("%s: #define %s not found" % (path, name))


def typedef(path, name):
    """typedef char name[ ... ]; (the compile-time checks)"""
    source = text(path)
    match = re.search(r"^typedef char %s\[.*?\];\n" % re.escape(name), source, re.S | re.M)
    if not match:
        raise SystemExit("%s: typedef %s not found" % (path, name))
    return match.group(0)


def function(path, signature):
    """a function from the line that begins with signature to its closing brace"""
    source = text(path)
    match = re.search(r"^%s.*?^\}\n" % re.escape(signature), source, re.S | re.M)
    if not match:
        raise SystemExit("%s: function %s not found" % (path, signature))
    return match.group(0)


def between(path, first, last):
    source = text(path)
    start = source.index(first)
    end = source.index(last, start)
    return source[start:end]


def enum_member(path, name):
    source = text(path)
    match = re.search(r"^\t%s = [^\n]*,\n" % re.escape(name), source, re.M)
    if not match:
        raise SystemExit("%s: enum member %s not found" % (path, name))
    return "enum\n{\n" + match.group(0) + "};\n"


def enum_block(path, name):
    source = text(path)
    match = re.search(r"^enum %s\n\{.*?^\};\n" % re.escape(name), source, re.S | re.M)
    if not match:
        raise SystemExit("%s: enum %s not found" % (path, name))
    return match.group(0)


def main(output):
    d = "port/linux/game/network_distributed.c"
    parts = []

    def add(title, body):
        parts.append("/* ---------- %s */\n\n%s\n" % (title, body))

    # the original's
    add("message_header.h, byte_swapping.h", enum_block("source/bungie_net/common/message_header.h", "message_header_byte_order") +
        "".join(enum_member("source/memory/byte_swapping.h", n) for n in ("_2byte", "_4byte")))
    add("message_header.c", function("source/bungie_net/common/message_header.c", "void build_message_header(") +
        "\n" + function("source/bungie_net/common/message_header.c", "void byte_swap_message_header("))
    add("data_encoding.c (its byte_swap_memory is a plain swap in the test)",
        struct("source/memory/data_encoding.h", "data_encoding_state") + "\n" +
        function("source/memory/data_encoding.c", "boolean data_encode_memory("))
    # the game's structures
    add("game.h, players.h", between("source/game/game.h", "struct slayer_statistics\n", "\n/* ---------- prototypes/GAME_STATISTICS.C") +
        struct("source/game/players.h", "player_action"))
    add("enums", enum_member(d, "MAXIMUM_STATISTICS_PER_MESSAGE") + enum_member(d, "MAXIMUM_PINGS_PER_MESSAGE") +
        enum_member(d, "MAXIMUM_SENDERS") + enum_member(d, "HOST_SENDER") +
        between(d, "/* struct distributed_unit_state flags */", "/* world units: how far a client's own"))
    # network_distributed.h and .c
    add("network_distributed.h", enum_member("source/networking/network_connection.h", "DATAGRAM_MAXIMUM_SIZE") +
        between("port/linux/game/network_distributed.h", "/* the messages, all of the game's", "/* where a message goes */") +
        enum_member("port/linux/game/network_distributed.h", "NO_PLAYER") +
        define("port/linux/game/network_distributed.h", "DISTRIBUTED_UNIT_SCALE") +
        struct("port/linux/game/network_distributed.h", "distributed_message_header") +
        struct("port/linux/game/network_distributed.h", "distributed_vector") +
        define("port/linux/game/network_distributed.h", "DISTRIBUTED_MESSAGE_HEADER_LAYOUT") +
        between("port/linux/game/network_distributed.h", "/* (what goes on the wire, as it is on every machine) */",
                "/* ---------- macros */") +
        between("port/linux/game/network_distributed.h", "/* the entries of a type that fit one unreliable message",
                "/* ---------- prototypes/NETWORK_DISTRIBUTED.C */"))
    # the modules'
    o = "port/linux/game/network_objects.c"
    add("network_objects.c",
        "".join(struct(o, n) + "\n" for n in ("distributed_object_change", "distributed_object_state", "distributed_inventory",
                                              "distributed_damage_animation")) +
        "".join(define(o, n) for n in ("OBJECT_CHANGE_LAYOUT", "OBJECT_STATE_LAYOUT", "INVENTORY_LAYOUT",
                                       "DAMAGE_ANIMATION_LAYOUT")) +
        "".join(typedef(o, n) for n in ("distributed_object_change_layout_assert", "distributed_object_state_layout_assert",
                                        "distributed_inventory_layout_assert", "distributed_damage_animation_layout_assert")) +
        function(o, "char const *network_objects_entry_layout("))
    m = "port/linux/game/network_damage.c"
    add("network_damage.c",
        "".join(struct(m, n) + "\n" for n in ("distributed_damage", "distributed_damage_event", "distributed_hit_report")) +
        "".join(define(m, n) for n in ("DISTRIBUTED_DAMAGE_LAYOUT", "DISTRIBUTED_DAMAGE_EVENT_LAYOUT",
                                       "DISTRIBUTED_HIT_REPORT_LAYOUT")) +
        "".join(typedef(m, n) for n in ("distributed_damage_layout_assert", "distributed_damage_event_layout_assert",
                                        "distributed_hit_report_layout_assert")) +
        function(m, "char const *network_damage_entry_layout("))
    a = "port/linux/game/network_actors.c"
    add("network_actors.c",
        "".join(struct(a, n) + "\n" for n in ("distributed_actor_state", "distributed_actor_damage")) +
        "".join(define(a, n) for n in ("ACTOR_STATE_LAYOUT", "ACTOR_DAMAGE_LAYOUT")) +
        "".join(typedef(a, n) for n in ("distributed_actor_state_layout_assert", "distributed_actor_damage_layout_assert")) +
        function(a, "char const *network_actors_entry_layout("))
    c = "port/linux/game/network_coop.c"
    add("network_coop.c",
        enum_member("source/scenario/scenario.h", "MAXIMUM_OBJECT_NAMES_PER_SCENARIO") +
        enum_member(c, "OBJECT_NAME_BYTES") +
        define("source/game/game_allegiance.h", "GAME_ALLEGIANCE_BITVECTOR_LONGS") +
        struct("source/rasterizer/rasterizer_cinematics.h", "rasterizer_screen_effect_port_state") + "\n" +
        "".join(struct(c, n) + "\n" for n in ("distributed_coop_presentation", "distributed_coop_event",
                                              "distributed_coop_device_group", "distributed_coop_object_names",
                                              "distributed_coop_object_transform", "distributed_coop_object_look",
                                              "distributed_coop_device_state")) +
        "".join(define(c, n) for n in ("COOP_PRESENTATION_LAYOUT", "COOP_EVENT_LAYOUT", "COOP_DEVICE_GROUP_LAYOUT",
                                       "COOP_OBJECT_NAMES_LAYOUT", "COOP_OBJECT_TRANSFORM_LAYOUT",
                                       "COOP_OBJECT_LOOK_LAYOUT", "COOP_DEVICE_STATE_LAYOUT",
                                       "COOP_SCREEN_EFFECT_LAYOUT")) +
        "".join(typedef(c, n) for n in ("distributed_coop_presentation_layout_assert", "distributed_coop_event_layout_assert",
                                        "distributed_coop_device_group_layout_assert",
                                        "distributed_coop_object_names_layout_assert",
                                        "distributed_coop_object_transform_layout_assert",
                                        "distributed_coop_object_look_layout_assert",
                                        "distributed_coop_device_state_layout_assert",
                                        "distributed_coop_screen_effect_layout_assert")) +
        function(c, "char const *network_coop_entry_layout("))
    add("network_distributed.c: the structure BSP's",
        struct(d, "distributed_structure_bsp") + define(d, "DISTRIBUTED_STRUCTURE_BSP_LAYOUT") +
        typedef(d, "distributed_structure_bsp_layout_assert"))
    v = "port/linux/game/network_voice.c"
    add("network_voice.c",
        "".join(struct(v, n) + "\n" for n in ("distributed_voice_up", "distributed_voice_down", "distributed_voice_config")) +
        "".join(define(v, n) for n in ("VOICE_UP_LAYOUT", "VOICE_DOWN_LAYOUT", "VOICE_CONFIG_LAYOUT", "VOICE_TO_WIRE")) +
        "".join(typedef(v, n) for n in ("distributed_voice_up_layout_assert", "distributed_voice_down_layout_assert",
                                        "distributed_voice_config_layout_assert")))
    add("network_distributed.c: the messages, the layouts, the wire",
        between(d, "/* a player's unit, as this machine has it", "/* messages of entries of a size of their own"))
    add("network_distributed.c: writing and reading",
        "NET_WIRE_BEFORE_FUNCTIONS\n\n" + function(d, "static byte *distributed_put(") + "\n" + function(d, "static boolean distributed_take(") + "\n" +
        function(d, "static void distributed_batch_flush(") + "\n" +
        function(d, "static word distributed_batch_room(") + "\n" +
        function(d, "static void distributed_batch_append(") + "\n" +
        function(d, "static void distributed_batch_add(") + "\n" +
        function(d, "static void distributed_fill_header(") + "\n" +
        function(d, "static word distributed_entry_size(") + "\n" +
        function(d, "void distributed_send_to_machine(") + "\n" +
        function(d, "void distributed_send_to_machine_reliably(") + "\n" +
        function(d, "static word distributed_unit_state_write(") + "\n" +
        function(d, "static word distributed_unit_state_read(") + "\n" +
        function(d, "static word distributed_relayed_action_write(") + "\n" +
        function(d, "static word distributed_relayed_action_read("))
    Path(output).parent.mkdir(parents=True, exist_ok=True)
    Path(output).write_text("/* made by tools/net_wire_extract.py from the game's sources */\n\n" + "\n".join(parts))


if __name__ == "__main__":
    main(sys.argv[1])
