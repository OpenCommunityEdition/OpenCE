#!/usr/bin/env python3
"""The wire format of HALO_PORT_NETWORK_VERSION (port/linux/network_format.txt).

Taken from the sources: the game's message numbers, sizes and fields
(network_messages.c, .h) and the distributed netcode's (network_*.c, .h,
map_share*.c, .h): the enums with _distributed_ members, the DISTRIBUTED_
constants, the structs sent whole (the _message structs, those whose size is
taken, and the structs in them) as hashes, and the constants their arrays
are sized by, as their definitions. An entry the netcode packs itself (a
unit's state, a relayed action) is covered by its flags and its least and
largest sizes only.

test_linux_port.py fails when the sources differ from the record. An item
changed or removed is one older builds cannot read: raise the version and
say why in NETCODE.md. An item added (a message kind, a bit) may be one they
ignore. Then record it: network_format.py --record
"""

import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / "port/linux/network_format.txt"


def _blocks(text, opener):
    """each declaration (to its semicolon) that starts with the opener"""
    for match in re.finditer(opener, text, re.M):
        end, depth = text.index("{", match.start()), 0
        while True:
            depth += {"{": 1, "}": -1}.get(text[end], 0)
            end += 1
            if not depth:
                break
        yield text[match.start() : text.index(";", end) + 1]


def _hash(text):
    return hashlib.sha256(" ".join(text.split()).encode()).hexdigest()[:12]


def items():
    """the wire format's items, name to value"""
    paths = sorted((ROOT / "port/linux/game").glob("network_*.[ch]"))
    paths += sorted((ROOT / "port/linux/game").glob("map_share*.[ch]"))
    paths += [ROOT / "source/networking/network_messages.c", ROOT / "source/networking/network_messages.h"]
    texts = [re.sub(r"/\*.*?\*/|//[^\n]*", " ", path.read_text(encoding="latin-1"), flags=re.S) for path in paths]
    everything = "\n".join(texts)
    result, structs = {}, {}

    def add(name, value):
        assert name not in result, f"{name} is declared twice"
        result[name] = str(value)

    for text in texts:
        for block in _blocks(text, r"^struct distributed_\w+\s*\{"):
            structs[re.match(r"struct (\w+)", block).group(1)] = block
        for block in _blocks(text, r"^enum\b[^;{]*\{"):
            if "_distributed_" not in block and "network_game_message_type" not in block:
                continue
            value = -1
            for member in filter(None, map(str.strip, block[block.index("{") + 1 : block.rindex("}")].split(","))):
                name, _, expression = (part.strip() for part in member.partition("="))
                value = int(expression, 0) if expression else value + 1
                if not name.startswith("NUMBER_OF_"):
                    add(f"enum {name}", value)
        for name, value in re.findall(r"^#define (DISTRIBUTED_\w+)\s+(.*?)\s*$", text, re.M):
            add(f"define {name}", value)
    sent = {name for name in structs if name.endswith("_message") or f"sizeof(struct {name})" in everything}
    while True:
        bodies = " ".join(structs[name].split("{", 1)[1] for name in sent)
        held = structs.keys() & set(re.findall(r"\bstruct (distributed_\w+)", bodies))
        if held <= sent:
            break
        sent |= held
    for name in sent:
        add(f"struct {name}", _hash(structs[name]))
    # the constants the structs' arrays are sized by, and the netcode's
    # DISTRIBUTED_ sizes in enums of no name, as their definitions' text, with
    # the names those use in turn
    bounds = set()
    for name in sent:
        for size in re.findall(r"\[([^\]]+)\]", structs[name].split("{", 1)[1]):
            bounds |= set(re.findall(r"\b[A-Z_][A-Z0-9_]*\b", size))
    bounds |= set(re.findall(r"^\s*(DISTRIBUTED_\w+)\s*=", everything, re.M)) - {
        key.split(" ", 1)[1] for key in result if key.startswith("enum ")
    }
    headers = None
    pending, done = sorted(bounds), set()
    while pending:
        name = pending.pop()
        if name in done:
            continue
        done.add(name)
        pattern = rf"^#define {name}\b[ \t]*((?:[^\n]*\\\n)*[^\n]*)|\b{name}[ \t]*=(?!=)([^,{{}};]*)"
        found = re.findall(pattern, everything, re.M)
        if not found:
            if headers is None:
                headers = "\n".join(
                    re.sub(r"/\*.*?\*/|//[^\n]*", " ", path.read_text(encoding="latin-1"), flags=re.S)
                    for root in ("source", "port/linux/include", "port/linux/game", "port/linux/src")
                    for path in sorted((ROOT / root).rglob("*.h"))
                )
            found = re.findall(pattern, headers, re.M)
        if not found:
            # (an enum's member counted, NUMBER_OF_ at its end: the enum)
            found = [
                ("enum " + _hash(block), "")
                for text in (everything, headers)
                for block in _blocks(text, r"\benum\b[^;{]*\{")
                if re.search(rf"\b{name}\s*[,}}]", block)
            ][:1]
        if not found:
            assert name not in bounds, f"{name}: a size of the wire format with no definition found"
            continue
        values = sorted({" ".join((define or expression).replace("\\\n", " ").split()) for define, expression in found})
        add(f"bound {name}", " | ".join(values))
        pending += re.findall(r"\b[A-Z_][A-Z0-9_]*\b", " ".join(values))
    messages = texts[-2]
    for name, size in re.findall(r"^DEFINE_NETWORK_GAME_MESSAGE\((\w+), (.*)\);", messages, re.M):
        add(f"size {name}", " ".join(size.split()))
    for fields, name in re.findall(r"\t\{\n((?:\t\t.*\n)*?)\t\},\n\tNETWORK_GAME_MESSAGE_DEFINITION\((\w+),", messages):
        add(f"fields {name}", _hash(fields))
    return result


def version():
    limits = (ROOT / "port/linux/include/halo_port_limits.h").read_text()
    return int(re.search(r"#define HALO_PORT_NETWORK_VERSION (\d+)", limits).group(1))


def recorded():
    lines = [line.split(" ", 2) for line in RECORD.read_text().splitlines()]
    return int(lines[0][1]), {f"{kind} {name}": value for kind, name, value in lines[1:]}


def changes(old, new):
    """(changed or removed, added)"""
    return sorted(name for name in old if old[name] != new.get(name)), sorted(name for name in new if name not in old)


def check():
    """what is wrong with the record, as lines to show"""
    problems = []
    recorded_version, old = recorded()
    breaking, added = changes(old, items())
    for names, what in (
        (breaking, "changed or removed (older builds cannot read them: raise the version)"),
        (added, "added (raise the version unless older builds ignore them)"),
    ):
        problems += [what + ":"] + [f"  {name}" for name in names] if names else []
    if recorded_version != version():
        problems.append(f"recorded for version {recorded_version}, the sources' is {version()}")
    if problems:
        problems.append("then record them: python tools/network_format.py --record")
    netcode = (ROOT / "port/linux/NETCODE.md").read_text()
    if not re.search(rf"\bversion\s+{version()}\b", netcode):
        problems.append(f"port/linux/NETCODE.md's versions say nothing of version {version()}")
    return problems


def record():
    new = items()
    recorded_version, old = recorded()
    breaking = changes(old, new)[0]
    if breaking and recorded_version == version():
        sys.exit("changed or removed: " + ", ".join(breaking) + "\nraise HALO_PORT_NETWORK_VERSION first")
    RECORD.write_text(f"version {version()}\n" + "".join(f"{name} {new[name]}\n" for name in sorted(new)))


if __name__ == "__main__":
    if sys.argv[1:] == ["--record"]:
        record()
    else:
        print("\n".join(check()) or "the wire format is the one recorded")
