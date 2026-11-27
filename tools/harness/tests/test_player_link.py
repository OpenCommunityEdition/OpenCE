"""A Discord link's pass and a dedicated server's netlist (docs/master_protocol.md, "The netlist", "Player keys,
links and passes") against the master's test vectors: a pass read and its master's signature, a connection's binding
and the player key's proof, a server's whole check of a pass at the door, the link page's code and the texts a link
and a pass request sign, and which master's pass a join shows (port/linux/src/player_link.c); a netlist read, the
addresses in it, and vpn_allow.txt's
ranges (port/linux/src/server_netlist.c); on the real SHA-256 and Ed25519 (p2p_crypto.c, Monocypher).

The vectors are test_master_protocol.py's (the master's testdata/vectors.json, else the copy here)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run  # noqa: E402
from harness.tests.test_master_protocol import c_string, crypto_includes, key_hex, vectors  # noqa: E402

CASES = ["pass-read", "pass-signed", "proof", "check", "passes-refused", "link-texts", "netlist", "netlists-refused",
         "ranges-text", "next-pass"]

SOURCES = {
    "port/linux/src/p2p.c": ["p2p_hex"],
    "port/linux/src/player_link.c": ["get_u64", "player_link_pass_read", "player_link_pass_signed",
                                     "player_link_binding", "player_link_proof_message", "player_link_proof_check",
                                     "player_link_code", "player_link_request_text", "player_link_check",
                                     "player_link_discord_created", "listed_by", "player_link_next_pass"],
    "port/linux/src/server_netlist.c": ["get_u32", "get_u64", "server_netlist_read", "server_netlist_ranges_contain",
                                        "read_address", "compare_pairs", "server_netlist_parse_ranges"],
}

# a fault in the code, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "pass-any-format": (("|| pass[8] != 1)", ")"), "player_link_pass_read", "passes-refused"),
    "pass-name-raw": (("character >= 0x20 && character < 0x7F ? (char)character : '?'", "(char)character"),
                      "player_link_pass_read", "pass-read"),
    "pass-unlabelled": (('memcpy(message, "opence-pass-1", 13);', 'memset(message, 0, 13);'),
                        "player_link_pass_signed", "pass-signed"),
    "binding-unlabelled": (('"opence-link-binding-1", 21', '"opence-link-binding-1", 20'), "player_link_binding",
                           "proof"),
    "proof-unbound": (("memcpy(message + 19 + 32, binding, PLAYER_LINK_BINDING_SIZE);",
                       "memset(message + 19 + 32, 0, PLAYER_LINK_BINDING_SIZE);"), "player_link_proof_message", "proof"),
    "check-any-master": (("if (index == master_count || !player_link_pass_signed(pass, master_keys[index]))",
                          "if (index == master_count && 0)"), "player_link_check", "check"),
    "check-no-expiry": (("|| fields->expires + CLOCK_SLACK_SECONDS < now", ""), "player_link_check", "check"),
    "check-no-proof": (("return player_link_proof_check(pass, binding, proof);", "return 1;"), "player_link_check",
                       "check"),
    "code-off-by-one": (("(27 - 5 * index)", "(26 - 5 * index)"), "player_link_code", "link-texts"),
    "request-reordered": (('"%s\\n%s\\n%s\\n%llu", prefix, key_id, key, ts', '"%s\\n%s\\n%s\\n%llu", prefix, key, key_id, ts'),
                          "player_link_request_text", "link-texts"),
    "netlist-touching": (("get_u32(range - 4) >= first - 1", "get_u32(range - 4) >= first"), "server_netlist_read",
                         "netlists-refused"),
    "netlist-reserved": (("if (body[9] || body[10] || body[11])", "if (0)"), "server_netlist_read",
                         "netlists-refused"),
    "netlist-contains-off": (("return low < count && get_u32(ranges + 8 * low) <= address;",
                              "return low < count && get_u32(ranges + 8 * low) < address;"),
                             "server_netlist_ranges_contain", "netlist"),
    "next-pass-any-master": (("(!server_count || listed_by(key_ids[index], server_key_ids, server_count))", "1"),
                             "player_link_next_pass", "next-pass"),
    "next-pass-repeats": (("index = after < 0 ? 0 : after + 1;", "index = after < 0 ? 0 : after;"),
                          "player_link_next_pass", "next-pass"),
    "next-pass-names-none": (("*needed = index;", "*needed = -1;"), "player_link_next_pass", "next-pass"),
    "ranges-never-merged": (("if (first <= *previous_last || first - 1 == *previous_last)",
                             "if (first <= *previous_last)"), "server_netlist_parse_ranges", "ranges-text"),
}


def vectors_include(v):
    """the vectors this test reads, as C"""
    discord_id = int(v["pass"]["discord_id"])
    values = {
        "MASTER_PUBLIC_KEY": key_hex(v["master"]["public_key"]), "MASTER_KEY_ID_BYTES": v["master"]["key_id_bytes"],
        "PUBLISHER_PUBLIC_KEY": key_hex(v["publisher"]["public_key"]),
        "PLAYER_SEED": v["player"]["seed"], "PLAYER_PUBLIC_KEY": key_hex(v["player"]["public_key"]),
        "LINK_TS": v["link"]["ts"], "LINK_SIGNED_TEXT": v["link"]["signed_text"], "LINK_SIG": v["link"]["sig"],
        "LINK_CODE": v["link"]["code"],
        "PASS_REQUEST_TS": v["pass_request"]["ts"], "PASS_REQUEST_SIGNED_TEXT": v["pass_request"]["signed_text"],
        "PASS_REQUEST_SIG": v["pass_request"]["sig"],
        "PASS_BYTES": v["pass"]["bytes"], "PASS_SIGNED_MESSAGE": v["pass"]["signed_message"],
        "PASS_PLAYER_PUBLIC_KEY": v["pass"]["player_public_key"], "PASS_DISCORD_ID": v["pass"]["discord_id"],
        "PASS_DISCORD_NAME": v["pass"]["discord_name"],
        "PROOF_SESSION_SECRET": v["pass_proof"]["session_secret"], "PROOF_BINDING": v["pass_proof"]["binding"],
        "PROOF_MESSAGE": v["pass_proof"]["message"], "PROOF_PROOF": v["pass_proof"]["proof"],
        "PROOF_LAN_BINDING": v["pass_proof"]["lan_binding"], "PROOF_LAN_MESSAGE": v["pass_proof"]["lan_message"],
        "PROOF_LAN_PROOF": v["pass_proof"]["lan_proof"],
        "NETLIST_BODY": v["netlist"]["body"],
    }
    text = "".join(f"#define VECTOR_{name} {c_string(value)}\n" for name, value in values.items())
    text += (f"#define VECTOR_PASS_FLAGS {v['pass']['flags']}\n#define VECTOR_PASS_ISSUED {v['pass']['issued']}ULL\n"
             f"#define VECTOR_PASS_EXPIRES {v['pass']['expires']}ULL\n"
             f"#define VECTOR_DISCORD_CREATED {((discord_id >> 22) + 1420070400000) // 1000}ULL\n"
             f"#define VECTOR_NETLIST_GENERATED {v['netlist']['generated']}ULL\n"
             f"#define VECTOR_NETLIST_SERIAL {v['netlist']['serial']}ULL\n")
    text += "static const char *const vector_netlist_ranges[][2] =\n{\n"
    text += "".join(f"\t{{ {c_string(first)}, {c_string(last)} }},\n" for first, last in v["netlist"]["ranges"])
    text += "};\nstatic const struct vector_bytes vector_passes_refused[] =\n{\n"
    text += "".join(f"\t{{ {c_string(each['name'])}, {c_string(each['bytes'])} }},\n" for each in v["passes_refused"])
    text += "};\nstatic const struct vector_bytes vector_netlists_refused[] =\n{\n"
    text += "".join(f"\t{{ {c_string(each['name'])}, {c_string(each['body'])} }},\n"
                    for each in v["netlist"]["refused"])
    return text + "};\n"


def generated(fault=None, faulty_function=None):
    link = read("port/linux/src/player_link.c")
    text = f"enum {{ CLOCK_SLACK_SECONDS = {constant(link, 'CLOCK_SLACK_SECONDS')} }};\n"
    for path, names in SOURCES.items():
        source = read(path)
        for name in names:
            code = function(source, name)
            if fault and name == faulty_function:
                code = mutated(code, *fault)
            # (each file's own get_u64: the netlist's renamed, as are its calls)
            if path.endswith("server_netlist.c"):
                code = code.replace("get_u64", "netlist_get_u64").replace("get_u32", "netlist_get_u32")
            text += code + "\n"
    return crypto_includes() + (("player_link.h", read("port/linux/src/player_link.h")),
                                ("server_netlist.h", read("port/linux/src/server_netlist.h")),
                                ("vectors.inc", vectors_include(vectors())), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("player_link", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("player_link", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
