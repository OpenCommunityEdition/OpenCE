"""List what the scripts of Custom Edition maps use that this build lacks.

Xbox maps are read too, so a campaign level's scripts can be written out.

A Custom Edition map's compiled scripts keep the name of every function they
call and every engine global they use (custom_edition_scripts.c finds them
again by name). This reads those names out of each map's scenario and checks
them against this build's tables, source/hs/hs.c's functions and
source/hs/hs_globals_external.c's globals.

    python tools/custom_edition_script_names.py [--all] [--scripts] MAP_OR_FOLDER...

It prints each missing name with the maps that use it, most used first. With
--all, every name used is listed with whether this build has it. With
--scripts, each map's scripts are listed too: name, type and the missing
names each one reaches. Nothing is written.
"""

import argparse
import collections
from pathlib import Path
import re
import struct
import sys

from custom_edition_tag_footprints import Cache, CUSTOM_EDITION_VERSION, XBOX_VERSION, read_cache

SOURCE = Path(__file__).resolve().parent.parent / "source" / "hs"

# where the scripts are in a scenario (scenario_definitions.h)
SCENARIO_SYNTAX_DATA = 0x474
SCENARIO_STRING_CONSTANTS = 0x488
SCENARIO_SCRIPTS = 0x49C
SCENARIO_GLOBALS = 0x4A8
TAG_DATA_ADDRESS = 12
DATA_ARRAY_BYTES = 0x38
SYNTAX_NODE_BYTES = 20
SCRIPT_BYTES = 92
GLOBAL_BYTES = 92
SCRIPT_TYPES = ("startup", "dormant", "continuous", "static", "stub")

# a syntax node's flags (hs_compile.c)
PRIMITIVE = 1 << 0
SCRIPT = 1 << 1
GLOBAL = 1 << 2
# an engine global's designator has this bit (hs_find_global_by_name)
EXTERNAL_GLOBAL = 1 << 15
# the type of a call's first child, which names its function
FUNCTION_NAME = 2
# the value types whose constants are written out as values (hs.h)
BOOLEAN, REAL, SHORT, LONG, STRING = 5, 6, 7, 8, 9
CONSTANT_TYPES = (BOOLEAN, REAL, SHORT, LONG, STRING)


# a function definition in hs.c: return type, flags, name, parser, evaluator,
# documentation, usage, parameter count and parameter types
C_STRING = r'(?:(?:"(?:[^"\\]|\\.)*"\s*)+|NULL)'
FUNCTION_DEFINITION = re.compile(
    r'_definition\s*=\s*\{\s*\{?\s*(_hs_\w+),\s*\w+,\s*"([^"]+)",\s*(\w+),\s*\w+,\s*'
    + C_STRING + r',\s*' + C_STRING + r',\s*(\d+)\s*,?\s*(?:\{([^}]*)\})?'
    # (the parameters past the first, in the definition's own array)
    r'(?:\s*,?\s*\},?\s*\{([^}]*)\})?', re.S)


def build_types():
    """hs.h's value types, by name, numbered as in the enumeration"""
    body = (SOURCE / "hs.h").read_text(encoding="latin-1")
    body = body[body.index("_hs_unparsed = 0"):body.index("NUMBER_OF_HS_TYPES")]
    return {name: number for number, name in enumerate(re.findall(r'(_hs_\w+)', body))}


def build_names():
    """This build's script functions, by name: (return type, parser,
    parameter types), and its engine globals' names."""
    types = build_types()
    functions = {}
    for return_type, name, parser, count, first, rest in FUNCTION_DEFINITION.findall(
            (SOURCE / "hs.c").read_text(encoding="latin-1")):
        parameters = [types.get(parameter.strip()) for parameter in (first + "," + rest).split(",") if parameter.strip()]
        functions[name.lower()] = (types.get(return_type), parser, parameters[:int(count)])
    globals_ = set(re.findall(r'definition\s*=\s*\{\s*"([a-z0-9_]+)"',
                              (SOURCE / "hs_globals_external.c").read_text(encoding="latin-1")))
    return functions, {name.lower() for name in globals_}


class MapScripts:
    """The function and engine global names one map's scripts use."""

    def __init__(self, path):
        self.cache = Cache(path)
        scenario = [key for key in self.cache.addresses if key[0] == "scnr"]
        if not scenario:
            raise ValueError(f"{path}: no scenario")
        self.scenario = self.cache.addresses[scenario[0]] - self.cache.base
        self.strings = self.data(SCENARIO_STRING_CONSTANTS)
        syntax = self.data(SCENARIO_SYNTAX_DATA)
        self.nodes = []
        if len(syntax) >= DATA_ARRAY_BYTES:
            maximum, size = struct.unpack_from("<hh", syntax, 32)
            count, = struct.unpack_from("<h", syntax, 46)
            if size != SYNTAX_NODE_BYTES:
                raise ValueError(f"{path}: syntax nodes of {size} bytes")
            for index in range(count):
                self.nodes.append(struct.unpack_from("<hhhhiii", syntax, DATA_ARRAY_BYTES + index * SYNTAX_NODE_BYTES))
        self.functions = collections.Counter()
        self.globals = collections.Counter()
        # each call's function and the types its arguments were compiled to
        self.calls = []
        for index, node in enumerate(self.nodes):
            kind, name = self.uses(node)
            if kind == "function":
                self.functions[name] += 1
                self.calls.append((name, self.argument_types(node)))
            elif kind == "global":
                self.globals[name] += 1

    def data(self, offset):
        size, = struct.unpack_from("<I", self.cache.tag_data, self.scenario + offset)
        address, = struct.unpack_from("<I", self.cache.tag_data, self.scenario + offset + TAG_DATA_ADDRESS)
        start = address - self.cache.base
        if not size or not 0 <= start <= len(self.cache.tag_data) - size:
            return b""
        return self.cache.tag_data[start:start + size]

    def block(self, offset, element_bytes):
        count, address = struct.unpack_from("<iI", self.cache.tag_data, self.scenario + offset)
        start = address - self.cache.base
        return [self.cache.tag_data[start + i * element_bytes:start + (i + 1) * element_bytes] for i in range(count)]

    def string(self, offset):
        if not 0 <= offset < len(self.strings):
            return None
        return self.strings[offset:self.strings.index(b"\0", offset)].decode("latin-1").lower()

    def uses(self, node):
        """("function" or "global", name) for a call or an engine global
        reference, else (None, None)"""
        header, index, value_type, flags, next_node, string_offset, data = node
        if not header:
            return None, None
        if not flags & (PRIMITIVE | SCRIPT):
            name_index = data & 0xFFFF
            if name_index < len(self.nodes) and self.nodes[name_index][2] == FUNCTION_NAME:
                return "function", self.string(self.nodes[name_index][5])
        elif flags & PRIMITIVE and flags & GLOBAL and data & EXTERNAL_GLOBAL:
            return "global", self.string(string_offset)
        return None, None

    def argument_types(self, call):
        """the types of a call's arguments: the nodes after its name"""
        types = []
        index = self.nodes[call[6] & 0xFFFF][4]
        while index != -1 and (index & 0xFFFF) < len(self.nodes) and len(types) < 64:
            node = self.nodes[index & 0xFFFF]
            types.append(node[2])
            index = node[4]
        return tuple(types)

    def reached(self, root):
        """the nodes under a script's root expression"""
        seen, pending = set(), [root & 0xFFFF]
        while pending:
            index = pending.pop()
            if index in seen or index >= len(self.nodes):
                continue
            seen.add(index)
            header, _, _, flags, next_node, _, data = self.nodes[index]
            if next_node != -1:
                pending.append(next_node & 0xFFFF)
            if not flags & PRIMITIVE and data != -1:
                pending.append(data & 0xFFFF)
        return seen

    def source(self, index, depth=0):
        """the expression at a node, written out as script source"""
        header, constant_type, value_type, flags, next_node, string_offset, data = self.nodes[index & 0xFFFF]
        if not flags & PRIMITIVE:
            parts, child = [], data
            while child != -1 and len(parts) < 256:
                parts.append(self.source(child, depth + 1))
                child = self.nodes[child & 0xFFFF][4]
            line = "(" + " ".join(parts) + ")"
            if len(line) + 4 * depth <= 100:
                return line
            return "(" + parts[0] + "".join("\n" + "    " * (depth + 1) + part for part in parts[1:]) + ")"
        # (a constant's value is in the low bytes its type needs; the rest is
        # whatever the compiler left there)
        if flags & GLOBAL or value_type not in CONSTANT_TYPES:
            text = self.string(string_offset)
            return text if text is not None else str(data)
        if value_type == BOOLEAN:
            return "true" if data & 0xFF else "false"
        if value_type == REAL:
            return f"{struct.unpack('<f', struct.pack('<i', data))[0]:g}"
        if value_type == SHORT:
            return str(struct.unpack("<h", struct.pack("<i", data)[:2])[0])
        if value_type == STRING:
            return '"' + (self.string(string_offset) or "") + '"'
        return str(data)

    def scripts(self):
        """(name, type, root expression) of each script"""
        for element in self.block(SCENARIO_SCRIPTS, SCRIPT_BYTES):
            name = element[:32].split(b"\0")[0].decode("latin-1")
            script_type, _, root = struct.unpack_from("<hhi", element, 32)
            yield name, SCRIPT_TYPES[script_type] if 0 <= script_type < len(SCRIPT_TYPES) else script_type, root


def maps_in(paths):
    for path in map(Path, paths):
        for candidate in sorted(path.glob("*.map")) if path.is_dir() else [path]:
            try:
                # (Xbox maps too: their scripts are laid out the same, which
                # is handy for reading a campaign level's)
                if read_cache(candidate)[0] in (CUSTOM_EDITION_VERSION, XBOX_VERSION):
                    yield candidate
            except (ValueError, OSError, struct.error):
                continue


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--all", action="store_true", help="list every name used, not only the missing ones")
    parser.add_argument("--scripts", action="store_true", help="list each map's scripts")
    parser.add_argument("--source", metavar="SCRIPT", action="append", default=[],
                        help="write out a script's source (any number; * for every script)")
    parser.add_argument("paths", nargs="+")
    arguments = parser.parse_args(argv)

    functions, globals_ = build_names()
    type_names = {number: name for name, number in build_types().items()}
    users = {"function": collections.defaultdict(set), "global": collections.defaultdict(set)}
    # calls whose arguments aren't this build's parameters: (function,
    # argument types) -> maps
    mismatches = collections.defaultdict(set)
    for path in maps_in(arguments.paths):
        try:
            scripts = MapScripts(path)
        except (ValueError, struct.error) as error:
            print(f"{path.name}: {error}", file=sys.stderr)
            continue
        for name in scripts.functions:
            users["function"][name].add(path.stem)
        for name in scripts.globals:
            users["global"][name].add(path.stem)
        for name, arguments_types in scripts.calls:
            definition = functions.get(name)
            if definition and definition[1] == "hs_macro_function_parse" and arguments_types != tuple(definition[2]):
                mismatches[(name, arguments_types)].add(path.stem)
        if arguments.scripts:
            print(f"{path.stem}: {len(scripts.nodes)} nodes")
            for name, script_type, root in scripts.scripts():
                missing = sorted({used for kind, used in map(scripts.uses, (scripts.nodes[i] for i in scripts.reached(root)))
                                  if kind and used and used not in (functions if kind == "function" else globals_)})
                print(f"  {script_type:<10} {name}" + (f"  (missing: {', '.join(missing)})" if missing else ""))
        for name, script_type, root in scripts.scripts():
            if name in arguments.source or "*" in arguments.source:
                print(f"\n; {path.stem}\n(script {script_type} {name}\n    {scripts.source(root, 1)})")

    for kind, known in (("function", functions), ("global", globals_)):
        names = sorted(users[kind], key=lambda name: (-len(users[kind][name]), name))
        shown = [name for name in names if arguments.all or name not in known]
        print(f"\n{kind}s{'' if arguments.all else ' this build lacks'}: {len(shown)}")
        for name in shown:
            mark = "" if not arguments.all else ("  " if name in known else "- ")
            print(f"  {mark}{name}  ({len(users[kind][name])}: {', '.join(sorted(users[kind][name])[:6])})")

    def signature(types):
        return ", ".join(type_names.get(number, str(number)).replace("_hs_type_", "") for number in types)

    print(f"\ncalls whose arguments differ from this build's parameters: {len(mismatches)}")
    for (name, arguments_types), maps in sorted(mismatches.items(), key=lambda item: (-len(item[1]), item[0])):
        print(f"  {name}({signature(arguments_types)}), here ({signature(functions[name][2])})"
              f"  ({len(maps)}: {', '.join(sorted(maps)[:6])})")


if __name__ == "__main__":
    main()
