"""The gametype files deploy/gametypes ships, checked against the rules the
server reads them by, taken from its sources: the rule table of
port/linux/src/server_rotation.c, the tables server_toml.c takes, the
built-in and OpenCE gametypes of port/linux/game/server_gametypes.c and the
game engine of each built-in's builder in source/game/game_engine.c. A key the
server would refuse (an alias, a rule in the wrong table, a value out of its
range) fails here, without a build or the game.

python -m pytest -q tools/test_deploy_gametypes.py
"""

import re
import tomllib
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
GAMETYPES = sorted((ROOT / "deploy/gametypes").glob("*.toml"))

# (game_modes.c's game_modes_gametype: the builder each of OpenCE's
# gametypes starts from; checked against server_gametypes.c's list below)
OPENCE_ENGINES = {
    "swat": "slayer", "infection": "slayer", "gun_game": "slayer", "hardcore": "slayer",
    "hardcore_ctf": "ctf", "hardcore_oddball": "oddball", "hardcore_king": "king",
}


def c_strings(text):
    return re.findall(r'"((?:[^"\\]|\\.)*)"', text)


def rule_table():
    """server_rotation.c's server_rule_definitions: key -> its rule."""
    source = (ROOT / "port/linux/src/server_rotation.c").read_text()
    words = {}
    for name, body in re.findall(r"static const char \*const (\w+)\[\]\s*=\s*\{(.*?)\};", source, re.S):
        words[name] = c_strings(re.sub(r"/\*.*?\*/", "", body, flags=re.S))
    table = source[source.index("server_rule_definitions[] ="):]
    table = table[:table.index("\n};")]
    rules = {}
    for kind, arguments in re.findall(r"RULE_(INTEGER|BOOLEAN|CHOICE)\((.*?)\),\s*$", table, re.S | re.M):
        key, alias, where = re.match(r'\s*"(\w+)",\s*(NULL|"\w+"),\s*"(\w+)"', arguments).groups()
        rest = [part.strip() for part in arguments.split(",")]
        rule = {"kind": kind.lower(), "alias": None if alias == "NULL" else alias.strip('"'), "table": where}
        if kind == "INTEGER":
            rule["minimum"], rule["maximum"] = int(rest[4]), int(rest[5])
        elif kind == "CHOICE":
            rule["choices"] = words[rest[4]]
        rules[key] = rule
    return rules


def engine_tables():
    source = (ROOT / "port/linux/src/server_toml.c").read_text()
    return c_strings(re.search(r"engine_tables\[\]\s*=\s*\{(.*?)\};", source, re.S).group(1))


def gametype_engines():
    """Each built-in, alias and OpenCE gametype's game engine (its table's name)."""
    gametypes = (ROOT / "port/linux/game/server_gametypes.c").read_text()
    engine = (ROOT / "source/game/game_engine.c").read_text()
    builders = {}
    for builder, body in re.findall(r"^struct game_variant \*(build_game_variant_\w+)\(\n(.*?)^\}", engine, re.S | re.M):
        found = re.search(r"result\.game_engine_index = game_engine_(\w+);", body)
        if found:
            builders[builder] = found.group(1)
    engines = {}
    for name, builder in re.findall(r'\{ "(\w+)", (build_game_variant_\w+), L"', gametypes):
        engines[name] = builders[builder]
    for alias, name in re.findall(r'\{ "(\w+)", "(\w+)" \}', gametypes):
        engines[alias] = engines[name]
    opence = c_strings(re.search(r"server_opence_gametypes\[\]\s*=\s*\{(.*?)\};", gametypes, re.S).group(1))
    assert sorted(opence) == sorted(OPENCE_ENGINES), "OpenCE's gametypes changed: update OPENCE_ENGINES"
    engines.update(OPENCE_ENGINES)
    return engines


def test_the_sources_parse():
    rules = rule_table()
    assert len(rules) > 40
    assert rules["vehicle_set"]["alias"] == "vehicles" and rules["vehicle_set"]["table"] == "universal"
    assert rules["team_vehicles"]["choices"] == ["gametype", "pc", "custom"]
    assert rules["time_limit"] == {"kind": "integer", "alias": "time", "table": "options", "minimum": 0,
                                   "maximum": 1440}
    assert engine_tables() == ["ctf", "slayer", "king", "oddball", "race"]
    engines = gametype_engines()
    assert engines["team_slayer"] == "slayer" and engines["juggernaut"] == "oddball"
    assert engines["koth"] == "king" and engines["team_rally"] == "race"


def test_there_are_gametype_files():
    assert GAMETYPES


@pytest.mark.parametrize("path", GAMETYPES, ids=lambda path: path.name)
def test_gametype_file(path):
    rules = rule_table()
    tables = engine_tables()
    engines = gametype_engines()
    assert re.fullmatch(r"[A-Za-z0-9_-]{1,58}", path.stem), "not a gametype file's name"
    data = tomllib.loads(path.read_text())
    engine_table = None
    for key, value in data.items():
        if key in ("base", "name"):
            assert isinstance(value, str), f"{key} is a string"
            continue
        assert isinstance(value, dict), f"{key} is not base or name: a rule goes in its table"
        assert key in ("universal", "options") or key in tables, f"[{key}] is no table of a gametype file"
        if key in tables:
            assert engine_table in (None, key), f"[{engine_table}] and [{key}]: a gametype has one game engine"
            engine_table = key
        for rule_key, rule_value in value.items():
            assert rule_key in rules, f"no rule is named {rule_key} (in [{key}]; an alias is not taken here)"
            rule = rules[rule_key]
            assert rule["table"] == key, f"{rule_key} goes in [{rule['table']}], not [{key}]"
            if rule["kind"] == "boolean":
                assert isinstance(rule_value, bool), f"{rule_key} takes true or false"
            elif rule["kind"] == "integer":
                assert isinstance(rule_value, int) and not isinstance(rule_value, bool), \
                    f"{rule_key} takes a whole number"
                assert rule["minimum"] <= rule_value <= rule["maximum"], \
                    f"{rule_key} takes {rule['minimum']} to {rule['maximum']}"
            else:
                assert rule_value in rule["choices"], f"{rule_key} takes one of {rule['choices']}"
    assert "base" in data, 'there is no base = "<gametype>"'
    base = data["base"].lower()
    assert base in engines, f"its base, {base}, is no built-in gametype"
    if engine_table:
        assert engine_table == engines[base], f"[{engine_table}] is not its base's game engine ({engines[base]})"
    if "name" in data:
        assert len(data["name"]) <= 11, "name is longer than 11 characters"
        assert all(" " <= character <= "~" for character in data["name"]), "name has a character that is not printable"
