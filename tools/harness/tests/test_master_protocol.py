"""The game's side of the master servers' protocol (docs/master_protocol.md) against the master's test vectors: a
signed list's checks, its entries taken, and masters.txt (master_list.c), the host's listing read and its signature
checked (p2p_lobby.c), a dedicated server's key, instance, server id, signed requests, its publishes and their answers,
and its probe's answer and limits (server_master.c), on the real SHA-256 and Ed25519 (p2p_crypto.c, Monocypher).

The vectors are the master's testdata/vectors.json (github.com/OpenCommunityEdition/opence-master-server, made by its
internal/vectors): read from a checkout beside this repository's when there is one, else from master_vectors.json
here, a copy of it taken on 2026-10-24 (copy it again when the master's vectors change: test_vectors_copy says so)."""

import json
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, ROOT, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

HERE = Path(__file__).resolve().parent
CHECKOUT_VECTORS = ROOT.parent / "opence-master-server" / "testdata" / "vectors.json"
COPIED_VECTORS = HERE / "master_vectors.json"
VECTORS_PATH = CHECKOUT_VECTORS if CHECKOUT_VECTORS.exists() else COPIED_VECTORS

CASES = ["crypto", "listing-signatures", "proof", "server-id", "key-file", "instance", "publish-request",
         "me-request", "probe", "probe-refusals", "list-verify", "list-refusals", "list-body", "masters-file",
         "masters-default", "masters-json", "listing-read", "listing-bounds", "publish-body", "publish-answers", "take-list", "list-large",
         "probe-nonces", "probe-sources"]

# the functions taken, in their files' order (each is defined before what calls it)
SOURCES = {
    "port/linux/src/p2p.c": ["p2p_hex"],
    "port/linux/src/p2p_lobby.c": ["sanitize", "get_long", "get_text", "listing_read", "listing_signed"],
    "port/linux/src/master_list.c": ["hex_digit", "master_url_allowed", "masters_file", "master_list_read_file", "master_keep",
                                     "master_list_read_defaults", "master_list_read", "master_list_read_json", "master_list_verify", "take_list"],
    "port/linux/src/server_master.c": ["hex_value", "seed_from_text", "key_file_path", "publisher_seed", "key_id_of",
                                       "server_master_publisher", "publisher_sign", "instance_name", "hex_text",
                                       "sign_request", "publish_body", "header_number", "logged_each_minute",
                                       "publish_answer", "publish_to", "master_start", "server_master_start",
                                       "probe_source_allowed", "server_master_probe"],
}

# a fault in the code, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "signs-path-before-method": (("master_key_id_text, method, path, now,", "master_key_id_text, path, method, now,"),
                                 "sign_request", "publish-request"),
    "signs-for-any-master": (("hex_text(master_key_id, SERVER_MASTER_KEY_ID_SIZE",
                              "hex_text(master.public_key, SERVER_MASTER_KEY_ID_SIZE"), "sign_request", "me-request"),
    "signs-without-version": (('"OpenCE-Master-v1\\n%s', '"OpenCE-Master\\n%s'), "sign_request", "me-request"),
    "server-id-separated": (('"%s%s", master.key_id, master.instance', '"%s-%s", master.key_id, master.instance'),
                            "master_start", "server-id"),
    "key-id-off-by-one": (("p2p_hex(digest, SERVER_MASTER_KEY_ID_SIZE, key_id);",
                           "p2p_hex(digest + 1, SERVER_MASTER_KEY_ID_SIZE, key_id);"), "key_id_of", "server-id"),
    "probe-signs-capitalised": (('memcpy(message, "probe", 5);', 'memcpy(message, "Probe", 5);'),
                                "server_master_probe", "probe"),
    "probe-takes-longer": (("size != PROBE_SIZE ||", "size < PROBE_SIZE ||"), "server_master_probe",
                           "probe-refusals"),
    "probe-repeats-nonce": (("master.nonce_seconds[index] == second && ", ""), "server_master_probe",
                            "probe-refusals"),
    "list-ignores-key-header": (("if (!key_header || strcmp(key_header, key_id))", "if (0)"),
                                "master_list_verify", "list-refusals"),
    "list-signs-less": (("p2p_ed25519_verify(key, body, (int)size, signature)",
                         "p2p_ed25519_verify(key, body, (int)size - 1, signature)"), "master_list_verify",
                        "list-verify"),
    "list-never-stale": (("(double)now - time_value > FRESH_SECONDS", "0"), "master_list_verify", "list-verify"),
    "list-any-master": (("!strcmp(text, key_id) &&", ""), "master_list_verify", "list-body"),
    "instance-keeps-leading-dash": (("while (length && (instance[0] == '-' || instance[0] == '_'))", "while (0)"),
                                    "instance_name", "instance"),
    "seed-takes-more-digits": (("return !*text || *text == ' '", "return 1 || *text == ' '"), "seed_from_text",
                               "key-file"),
    "masters-keep-slash": (("while (strlen(url) > 8 && url[strlen(url) - 1] == '/')", "while (0)"),
                           "master_list_read_file", "masters-file"),
    "masters-json-any-scheme": (('if (!strncmp(url, "https://", 8))\n\t\treturn 1;', 'if (1)\n\t\treturn 1;'),
                                "master_url_allowed", "masters-json"),
    "loopback-by-prefix": (("(!url[length] || url[length] == ':' || url[length] == '/')", "1"), "master_url_allowed",
                           "masters-file"),
    "masters-file-any-url": (("if (!master_url_allowed(url) ||", "if ("), "master_list_read_file", "masters-file"),
    "masters-file-not-default": (("return count ? count : master_list_read_defaults(urls, keys, maximum);",
                                  "return master_list_read_defaults(urls, keys, maximum);"), "master_list_read",
                                 "masters-default"),
    "masters-never-default": (("return count ? count : master_list_read_defaults(urls, keys, maximum);",
                               "return count;"), "master_list_read", "masters-default"),
    "masters-default-key": (('"ed25519:89f4eda71229f11f3538679f49ef7fb0c2b407b380093f7f4fe02105b92225aa"',
                             '"ed25519:89f4eda71229f11f3538679f49ef7fb0c2b407b380093f7f4fe02105b92225ab"'),
                            "master_list_read_defaults", "masters-default"),
    "masters-json-unbounded": (("for (index = 0; count < maximum && server_json_element",
                                "for (index = 0; server_json_element"), "master_list_read_json", "masters-json"),
    "list-taken-unchecked": (("p2p_sha256(key, MASTER_LIST_KEY_SIZE, digest);\n\tp2p_hex(digest, 8, key_id);",
                              "return 1;"), "master_list_verify", "list-refusals"),
    "list-any-protocol": (("&protocol) && protocol == 1 &&", "&protocol) &&"), "master_list_verify", "list-body"),
    "list-from-the-future": (("if (time_value - (double)now > FRESH_SECONDS)", "if (0)"), "master_list_verify",
                             "list-verify"),
    "list-verify-few-tokens": (("server_json_parse_tokens(&document, body, (int)size, 16, SERVER_JSON_MAXIMUM_LIST_TOKENS) ||",
                                "server_json_parse(&document, body, (int)size, 16) ||"), "master_list_verify",
                               "list-large"),
    "take-list-few-tokens": (("server_json_parse_tokens(&document, body, (int)size, 16, SERVER_JSON_MAXIMUM_LIST_TOKENS))",
                              "server_json_parse(&document, body, (int)size, 16))"), "take_list", "list-large"),
    "take-list-unbounded": (("for (index = 0; index < MAXIMUM_ENTRIES && server_json_element",
                             "for (index = 0; server_json_element"), "take_list", "list-large"),
    "take-list-bad-entry-ends-it": (("\t\t{\n\t\t\tcontinue;\n\t\t}\n\t\tlisting_size", "\t\t{\n\t\t\tbreak;\n\t\t}\n\t\tlisting_size"),
                                    "take_list", "take-list"),
    "take-list-publisher-raw": (("if ((unsigned char)publisher[character] < 0x20 ||",
                                 "if ((unsigned char)publisher[character] < 0x01 ||"), "take_list", "take-list"),
    "take-list-region-raw": (("if (!(isalnum((unsigned char)region[character]) || region[character] == '-'))", "if (0)"),
                             "take_list", "take-list"),
    "take-list-publisher-long": (("publisher[P2P_LISTING_PUBLISHER_SIZE] = 0;", "publisher[sizeof(publisher) - 1] = 0;"),
                                 "take_list", "take-list"),
    "take-list-other-master": (("dedicated,\n\t\t\tmaster_key_id);", 'dedicated,\n\t\t\t(const unsigned char *)"wrongmas");'),
                               "take_list", "take-list"),
    "take-list-never-dedicated": (('server_json_member(&document, entry, "dedicated")',
                                   'server_json_member(&document, entry, "no such field")'), "take_list", "take-list"),
    "listing-unlabelled": (("memcpy(data, signature_label, sizeof(signature_label) - 1);\n\tmemcpy(data + sizeof",
                            "memset(data, 0, sizeof(signature_label) - 1);\n\tmemcpy(data + sizeof"), "listing_signed",
                           "listing-read"),
    "listing-signs-less": (("bytes, (size_t)listing->signed_size);", "bytes, (size_t)listing->signed_size - 1);"),
                           "listing_signed", "listing-read"),
    "listing-any-format": (("bytes[2] != LISTING_FORMAT)", "0)"), "listing_read", "listing-bounds"),
    "listing-names-long": (("if (length > maximum || *offset + length > size)", "if (*offset + length > size)"),
                           "get_text", "listing-bounds"),
    "listing-trailing-taken": (("if (offset + P2P_SIGNATURE_SIZE != size)", "if (offset + P2P_SIGNATURE_SIZE > size)"),
                               "listing_read", "listing-bounds"),
    "listing-kind-lost": (("listing->kind = offset < size ? bytes[offset] : 0;", "listing->kind = 0;"), "listing_read",
                          "listing-read"),
    "publish-signs-bare-path": (('snprintf(signed_path, sizeof(signed_path), "%s%s", base_path ? base_path : "", path);',
                                 'snprintf(signed_path, sizeof(signed_path), "%s", path);'), "publish_to",
                                "publish-body"),
    "publish-port-unswapped": (("(long long)(unsigned short)(port << 8 | port >> 8)", "(long long)port"), "publish_body",
                               "publish-body"),
    "publish-never-dedicated": (('server_json_boolean(json, "dedicated", halo_server_mode() != 0);',
                                 'server_json_boolean(json, "dedicated", 0);'), "publish_body", "publish-body"),
    "publish-never-closed": (('server_json_boolean(json, "closed", closed);', 'server_json_boolean(json, "closed", 0);'),
                             "publish_body", "publish-body"),
    "withdrawn-never": (("if (!each->published)\n\t\t\treturn;", "return;"), "publish_to", "publish-body"),
    "retry-after-case": (("tolower((unsigned char)line[index]) == tolower((unsigned char)name[index])",
                          "line[index] == name[index]"), "header_number", "publish-answers"),
    "revoked-published-again": (("if (each->revoked || (!closed", "if ((!closed"), "publish_to", "publish-answers"),
    "clock-difference-unlogged": (('if (!strcmp(code, "clock") && has_master_time)', "if (0 && has_master_time)"),
                                  "publish_answer",
                                  "publish-answers"),
    "probe-any-master": (("if (!known)\n\t\treturn 0;", "if (0 && !known)\n\t\treturn 0;"), "server_master_probe",
                         "probe-refusals"),
    "probe-without-key": (("|| !master.has_key)\n\t\treturn 0;", ")\n\t\treturn 0;"), "server_master_probe",
                          "probe-refusals"),
    "probe-rate-unlimited": (("++master.probe_answers > PROBE_ANSWERS_PER_SECOND", "++master.probe_answers > 1000"),
                             "server_master_probe", "probe-sources"),
    "probe-sources-unlimited": (("return ++master.source_answers[index] <= PROBE_ANSWERS_PER_ADDRESS;", "return 1;"),
                                "probe_source_allowed", "probe-sources"),
    "probe-sources-one": (("if (master.source_seconds[index] != second)\n\t\t{", "if (1)\n\t\t{"),
                          "probe_source_allowed", "probe-sources"),
    "probe-nonce-last-only": (("for (index = 0; index < REMEMBERED_NONCES; index++)\n\t{\n\t\tif (master.nonce_seconds",
                               "for (index = (master.next_nonce + REMEMBERED_NONCES - 1) % REMEMBERED_NONCES; "
                               "index == (master.next_nonce + REMEMBERED_NONCES - 1) % REMEMBERED_NONCES; index++)"
                               "\n\t{\n\t\tif (master.nonce_seconds"), "server_master_probe", "probe-nonces"),
}

# a vector the code disagrees with: the case and why (docs/master_protocol.md)
DISAGREEMENTS = {}


def vectors():
    return json.loads(VECTORS_PATH.read_text(encoding="utf-8"))


def c_string(text):
    """A C string literal of ASCII text."""
    out = '"'
    for character in text:
        if character in '"\\':
            out += "\\" + character
        elif character == "\n":
            out += "\\n"
        elif " " <= character <= "~":
            out += character
        else:
            raise ValueError(f"not printable ASCII in a vector: {character!r}")
    return out + '"'


def key_hex(text):
    """The hex of an "ed25519:<hex>" public key."""
    assert text.startswith("ed25519:"), text
    return text[8:]


def vectors_include(v):
    """vectors.json as C: a #define a value, and the listings' table."""
    values = {
        "PUBLISHER_SEED": v["publisher"]["seed"], "PUBLISHER_PUBLIC_KEY": key_hex(v["publisher"]["public_key"]),
        "PUBLISHER_KEY_ID": v["publisher"]["key_id"],
        "MASTER_SEED": v["master"]["seed"], "MASTER_PUBLIC_KEY_TEXT": v["master"]["public_key"],
        "MASTER_PUBLIC_KEY": key_hex(v["master"]["public_key"]), "MASTER_KEY_ID": v["master"]["key_id"],
        "MASTER_KEY_ID_BYTES": v["master"]["key_id_bytes"],
        "HOST_SEED": v["host"]["seed"], "HOST_PUBLIC_KEY": key_hex(v["host"]["public_key"]),
        "HOST_KEY_ID": v["host"]["key_id"],
        "INSTANCE": v["instance"], "SERVER_ID_INPUT": v["server_id_input"], "SERVER_ID": v["server_id"],
        "PROBE_MASTER_KEY_ID": v["probe"]["master_key_id"], "PROBE_NONCE": v["probe"]["nonce"],
        "PROBE_PACKET": v["probe"]["packet"], "PROBE_SIGNED_MESSAGE": v["probe"]["signed_message"],
        "PROBE_ANSWER": v["probe"]["answer"],
        "LIST_BODY": v["list"]["body"], "LIST_SIGNATURE_HEADER": v["list"]["signature_header"],
        "LIST_KEY_HEADER": v["list"]["signature_key_header"], "LIST_TIME": v["list"]["signature_time_header"],
        "SMALL_ORDER_KEY": key_hex(v["small_order_key_refused"]),
        "PROOF_NONCE": v["proof_nonce"], "PROOF_SIGNATURE": v["proof_signature_hex"],
    }
    for prefix, request in (("PUBLISH", v["publish_request"]), ("ME", v["me_request"])):
        for field in ("master_key_id", "method", "path", "ts", "nonce", "body", "body_sha256", "canonical", "signature",
                      "authorization"):
            values[f"{prefix}_{field.upper()}"] = request[field]
    text = "".join(f"#define VECTOR_{name} {c_string(value)}\n" for name, value in values.items())
    text += "static const struct vector_listing vector_listings[] =\n{\n"
    for listing in v["listings"]:
        text += "\t{ " + ", ".join([c_string(listing["description"]), c_string(listing["host_seed"]),
                                    c_string(listing["host_public_key"]), c_string(listing["bytes"]),
                                    c_string(listing["base64"]), str(listing["size"]), str(listing["signed_size"]),
                                    c_string(listing["signed_message"]), c_string(listing["signature"]),
                                    "1" if listing["valid"] else "0"]) + " },\n"
    return text + "};\n"


def crypto_includes():
    """p2p_crypto.c, Monocypher and server_json.c (jsmn) as whole files, with the headers they include: the parts
    of p2p_internal.h and posix.h they need."""
    internal = read("port/linux/src/p2p_internal.h")
    section = internal[internal.index("/* ---------- p2p_crypto.c */"):]
    section = section[:section.index("/* ----------", 10)]
    public = read("port/linux/src/p2p.h")
    sizes = ", ".join(f"{name} = {constant(public, name)}" for name in (
        "P2P_LISTING_NAME_SIZE", "P2P_LISTING_MAP_SIZE", "P2P_LISTING_GAMETYPE_SIZE", "P2P_LISTING_PUBLISHER_SIZE",
        "P2P_LISTING_REGION_SIZE"))
    internal_stub = ("#include <stdint.h>\n"
                     f"enum {{ P2P_TOKEN_SIZE = {constant(internal, 'P2P_TOKEN_SIZE')}, {sizes} }};\n"
                     "void p2p_hex(const unsigned char *bytes, int size, char *text);\n" + section)
    posix = read("port/linux/src/posix.h")
    posix_stub = "typedef unsigned long posix_ulong;\n" + \
        re.search(r"^void posix_random_bytes\([^;]*\);", posix, re.M).group(0) + "\n"
    server_json = read("port/linux/src/server_json.c")
    if server_json.count('#include "../../third_party/jsmn/jsmn.h"') != 1:
        raise LookupError("server_json.c no longer includes jsmn as it did")
    server_json = server_json.replace('#include "../../third_party/jsmn/jsmn.h"', '#include "jsmn.h"')
    return (
        ("posix.h", posix_stub), ("p2p_internal.h", internal_stub),
        ("monocypher.h", read("port/third_party/monocypher/monocypher.h")),
        ("monocypher-ed25519.h", read("port/third_party/monocypher/monocypher-ed25519.h")),
        ("monocypher.inc", read("port/third_party/monocypher/monocypher.c")),
        ("monocypher_ed25519.inc", read("port/third_party/monocypher/monocypher-ed25519.c")),
        ("p2p_crypto.inc", read("port/linux/src/p2p_crypto.c")),
        ("jsmn.h", read("port/third_party/jsmn/jsmn.h")),
        ("server_json.h", read("port/linux/src/server_json.h")),
        ("server_json.inc", server_json),
        ("master_list.h", read("port/linux/src/master_list.h")),
        ("server_master.h", read("port/linux/src/server_master.h")),
    )


def generated(fault=None, faulty_function=None):
    lists = read("port/linux/src/master_list.c")
    publisher = read("port/linux/src/server_master.c")
    lobby = read("port/linux/src/p2p_lobby.c")
    state = re.search(r"^static struct\s*\{.*?^\} master;", publisher, re.M | re.S)
    if not state:
        raise LookupError("server_master.c's state (static struct { ... } master;) not found")
    label = re.search(r"^static const char signature_label\[\] = \"[^\"]*\";", lobby, re.M)
    if not label:
        raise LookupError("p2p_lobby.c's signature_label not found")
    config = ""
    for name in ("FRESH_SECONDS", "MAXIMUM_ENTRIES", "MAXIMUM_LISTING"):
        config += f"#define {name} {constant(lists, name)}\n"
    for name in ("PROBE_ANSWERS_PER_SECOND", "PROBE_ANSWERS_PER_ADDRESS", "REMEMBERED_NONCES", "REMEMBERED_SOURCES",
                 "PROBE_SIZE", "PROBE_ANSWER_SIZE", "MAXIMUM_ANSWER"):
        config += f"#define {name} {constant(publisher, name)}\n"
    config += structure(read("port/linux/src/update.h"), "update_response") + "\n"
    config += structure(publisher, "master") + "\n" + state.group(0) + "\n"
    config += enum_with(lobby, "LISTING_FORMAT = 1") + "\n" + label.group(0) + "\n" + structure(lobby, "listing") + "\n"
    text = ""
    for path, names in SOURCES.items():
        source = read(path)
        for name in names:
            code = function(source, name)
            if fault and name == faulty_function:
                code = mutated(code, *fault)
            text += code + "\n"
    return crypto_includes() + (("vectors.inc", vectors_include(vectors())), ("config.inc", config),
                                ("under_test.inc", text))


def test_vectors_copy():
    """The copy here is the master's vectors (when the master's repository is checked out beside this one)."""
    if not CHECKOUT_VECTORS.exists():
        pytest.skip("the master's repository is not checked out beside this one")
    assert json.loads(COPIED_VECTORS.read_text()) == json.loads(CHECKOUT_VECTORS.read_text()), \
        f"the master's vectors changed: copy {CHECKOUT_VECTORS} to {COPIED_VECTORS}"


@pytest.mark.parametrize("case", [pytest.param(case, marks=pytest.mark.xfail(reason=DISAGREEMENTS[case], strict=True))
                                  if case in DISAGREEMENTS else case for case in CASES])
def test_case(case):
    status, output = run(build("master_protocol", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("master_protocol", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
