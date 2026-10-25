"""The ragdolls of the stock game's bipeds (port/linux/game/ragdoll_bodies.c), built from their models' nodes and their
collision models' pieces as the maps in assets/maps have them, with port/assets/ragdolls/ragdolls.txt's overrides:
every biped with a collision model but those the overrides say have none gets a body; its parts follow the bones a body bends at (the
pelvis, spine, head, thighs, calves, upper arms and forearms of those that have them), each joined to an ancestor's
part; its shapes are the size of its pieces and its masses add up to the species'; its limits are within the host's.
The code under test is the sources' (the harness, tools/harness)."""

import functools
import math
import re
import struct
import sys
import zlib
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
MAPS = ROOT / "assets" / "maps"
sys.path.insert(0, str(ROOT / "tools"))
from harness import build, read, run  # noqa: E402
from harness.tests.test_ragdoll_bodies import generated  # noqa: E402

# where an Xbox map's tags are in memory
TAG_BASE = 0x803A6000
# offsets in the tags: the object's model and collision model references, the model's nodes, the collision model's
# nodes, a collision node's bsps, a bsp's vertices; and the elements' sizes
OBJECT_MODEL, OBJECT_COLLISION_MODEL = 0x28, 0x70
MODEL_NODES, MODEL_NODE_SIZE = 184, 156
COLLISION_MODEL_NODES, COLLISION_NODE_SIZE, COLLISION_NODE_BSPS = 0x28C, 64, 52
BSP_VERTICES, VERTEX_SIZE = 84, 16

SHAPES = {0: "sphere", 1: "capsule", 2: "box"}
# bones a ragdoll bends at, where a model has them and their pieces
LIMBS = (
    "pelvis",
    "spine",
    "head",
    "l thigh",
    "r thigh",
    "l calf",
    "r calf",
    "l upperarm",
    "r upperarm",
    "l forearm",
    "r forearm",
)


def maps():
    return sorted(path for path in MAPS.glob("*.map") if path.name != "ui.map") if MAPS.is_dir() else []


def tags_of(path):
    raw = path.read_bytes()
    data = raw[:2048] + zlib.decompress(raw[2048:])
    offset, size = struct.unpack_from("<II", data, 0x10)
    return data[offset : offset + size]


def profiles_of(path):
    """{model name: (nodes [(name, parent, x, y, z)], pieces [(name, [(x, y, z)])])} of a map's bipeds"""
    tags = tags_of(path)

    def at(address, layout):
        return struct.unpack_from("<" + layout, tags, address - TAG_BASE)

    def name_at(address, length=None):
        start = address - TAG_BASE
        end = start + length if length else tags.index(b"\0", start)
        return tags[start:end].split(b"\0")[0].decode("latin-1")

    def block(address):
        return at(address, "iI")

    index_address, _, _, count = at(TAG_BASE, "IIII")
    instances = {}
    for index in range(count):
        group, _, _, handle, name, address, _, _ = at(index_address + 32 * index, "IIIIIIII")
        instances[handle & 0xFFFF] = (struct.pack(">I", group).decode("latin-1"), name_at(name), address)
    profiles = {}
    for group, _, address in instances.values():
        if group != "bipd":
            continue
        model = instances.get(at(address + OBJECT_MODEL + 12, "I")[0] & 0xFFFF)
        collision = instances.get(at(address + OBJECT_COLLISION_MODEL + 12, "I")[0] & 0xFFFF)
        if not model or model[0] != "mode":
            continue
        node_count, node_address = block(model[2] + MODEL_NODES)
        nodes = []
        for node in range(node_count):
            element = node_address + MODEL_NODE_SIZE * node
            nodes.append((name_at(element, 32), at(element + 36, "h")[0], *at(element + 40, "fff")))
        pieces = []
        if collision and collision[0] == "coll":
            piece_count, piece_address = block(collision[2] + COLLISION_MODEL_NODES)
            for piece in range(piece_count):
                element = piece_address + COLLISION_NODE_SIZE * piece
                bsp_count, bsp_address = block(element + COLLISION_NODE_BSPS)
                points = []
                if bsp_count:
                    vertex_count, vertex_address = block(bsp_address + BSP_VERTICES)
                    points = [at(vertex_address + VERTEX_SIZE * v, "fff") for v in range(vertex_count)]
                pieces.append((name_at(element, 32), points))
        profiles[model[1]] = (nodes, pieces)
    return profiles


@functools.lru_cache(maxsize=None)
def profiles():
    found = {}
    for path in maps():
        for name, profile in profiles_of(path).items():
            found.setdefault(name, profile)
    return found


def c_string(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n\"\n\t\"") + '"'


def profiles_include(found):
    models, nodes, pieces, vertices = [], [], [], []
    for name, (model_nodes, model_pieces) in sorted(found.items()):
        models.append(f"\t{{{c_string(name)}, {len(nodes)}, {len(model_nodes)}, {len(pieces)}, {len(model_pieces)}}},")
        for node, parent, x, y, z in model_nodes:
            nodes.append(f"\t{{{c_string(node)}, {parent}, {x!r}f, {y!r}f, {z!r}f}},")
        for piece, points in model_pieces:
            pieces.append(f"\t{{{c_string(piece)}, {len(vertices) // 3}, {len(points)}}},")
            for point in points:
                vertices.extend(point)
    floats = ", ".join(f"{v!r}f" if math.isfinite(v) else "NAN" for v in vertices) or "0"
    return "\n".join(
        [
            "static struct profile_model const profile_models[] = {",
            *models,
            "};",
            "static struct profile_node const profile_nodes[] = {",
            *nodes,
            "};",
            "static struct profile_piece const profile_pieces[] = {",
            *pieces,
            "};",
            f"static real const profile_vertices[] = {{{floats}}};",
            f"static char const profile_overrides[] = {c_string(read('port/assets/ragdolls/ragdolls.txt'))};",
            "",
        ]
    )


@functools.lru_cache(maxsize=None)
def bodies():
    """{model name: None, or (mass, [part dict], [part of each node])}, as the sources build them"""
    found = profiles()
    executable = build("ragdoll_profiles", generated() + (("profiles.inc", profiles_include(found)),))
    status, output = run(executable, "all")
    assert status == 0, output
    result, current = {}, None
    for line in output.splitlines():
        words = line.split(" ")
        if words[0] == "MODEL":
            body = line[len("MODEL ") :]
            if body.endswith(" NONE"):
                result[body[: -len(" NONE")]] = current = None
                continue
            name, rest = body.rsplit(" BODY ", 1)
            current = result[name] = (float(rest.split()[1]), [], [])
        elif words[0] == "PART":
            values = [float(word) for word in words[1:]]
            current[1].append(
                {
                    "node": int(values[1]),
                    "parent": int(values[2]),
                    "shape": SHAPES[int(values[3])],
                    "size": values[4:7],
                    "center": values[7:10],
                    "mass": values[10],
                    "twist": values[11:13],
                    "swing": values[13:15],
                    "break": values[15],
                }
            )
        elif words[0] == "NODE":
            current[2].append(int(words[2]))
    return result


def sections():
    """{model name (lower case): [its override lines]}"""
    found, current = {}, None
    for line in read("port/assets/ragdolls/ragdolls.txt").splitlines():
        line = line.split("#", 1)[0].strip()
        if line.startswith("model "):
            current = found.setdefault(line[len("model ") :].lower(), [])
        elif line and current is not None:
            current.append(line)
    return found


needs_maps = pytest.mark.skipif(not maps(), reason="needs the stock maps in assets/maps")


@needs_maps
def test_every_biped_is_found():
    found = profiles()
    for name in (
        "characters\\marine\\marine",
        "characters\\elite\\elite",
        "characters\\grunt\\grunt",
        "characters\\jackal\\jackal",
        "characters\\hunter\\hunter",
        "characters\\cyborg\\cyborg",
        "characters\\floodcombat_human\\floodcombat_human",
        "characters\\flood_infection\\flood_infection",
    ):
        assert name in found, sorted(found)


@needs_maps
def test_bodies_unless_none():
    """(a cinematic biped without a collision model has no pieces to make a body of)"""
    found = profiles()
    overrides = sections()
    for name, body in bodies().items():
        none = "none" in overrides.get(name.lower(), []) or not any(points for _, points in found[name][1])
        assert (body is None) == none, f"{name}: {'a body' if body else 'no body'}"


@needs_maps
def test_masses_are_the_species():
    overrides = sections()
    for name, body in bodies().items():
        if body is None:
            continue
        masses = [float(line.split()[1]) for line in overrides.get(name.lower(), []) if line.startswith("mass ")]
        mass = masses[0] if masses else 80.0
        total = sum(part["mass"] for part in body[1])
        assert body[0] == pytest.approx(mass), name
        # (no part lighter than 1% of the whole: a little over, at most)
        assert mass * 0.999 <= total <= mass * 1.05, f"{name}: {total} kg of {mass}"
        assert all(part["mass"] >= 0.01 * mass - 1e-3 for part in body[1]), name


@needs_maps
def test_limbs_are_parts():
    found = profiles()
    for name, body in bodies().items():
        if body is None:
            continue
        nodes, pieces = found[name]
        with_geometry = {piece.lower() for piece, points in pieces if points}
        part_nodes = {nodes[part["node"]][0].lower() for part in body[1]}
        for node, *_ in nodes:
            lower = node.lower()
            if lower in with_geometry and any(lower.endswith(limb) for limb in LIMBS):
                assert lower in part_nodes, f"{name}: {node} moves with another part"


@needs_maps
def test_parts_join_ancestors():
    found = profiles()
    for name, body in bodies().items():
        if body is None:
            continue
        nodes = found[name][0]
        parts, node_parts = body[1], body[2]
        assert 2 <= len(parts) <= 32 and len(node_parts) == len(nodes), name
        assert parts[0]["node"] == 0 and parts[0]["parent"] == -1, name
        for index, part in enumerate(parts):
            assert node_parts[part["node"]] == index, f"{name}: part {index}"
            if index:
                assert 0 <= part["parent"] < index, f"{name}: part {index}'s parent {part['parent']}"
        for node, part in enumerate(node_parts):
            ancestor = node
            while ancestor != -1 and parts[part]["node"] != ancestor:
                ancestor = nodes[ancestor][1]
            assert ancestor != -1, f"{name}: {nodes[node][0]} moves with part {part}, not an ancestor's"


@needs_maps
def test_shapes_fit_their_pieces():
    found = profiles()
    for name, body in bodies().items():
        if body is None:
            continue
        nodes, pieces = found[name]
        points_of = {piece.lower(): points for piece, points in pieces if points}
        for part in body[1]:
            node = nodes[part["node"]][0]
            sizes = [size for size in part["size"] if size]
            assert all(0.01 - 1e-6 <= size <= 3.5 for size in sizes), f"{name}: {node}'s {part['shape']} {sizes}"
            points = points_of.get(node.lower())
            if not points:
                assert part["shape"] == "sphere" and part["center"] == [0, 0, 0], f"{name}: {node}"
                continue
            for axis in range(3):
                low = min(point[axis] for point in points)
                high = max(point[axis] for point in points)
                assert low - 1e-3 <= part["center"][axis] <= high + 1e-3, f"{name}: {node}'s centre"
            if part["shape"] == "capsule":
                radius, half = part["size"][:2]
                length = max(p[0] for p in points) - min(p[0] for p in points)
                assert abs(2 * (radius + half) - length) < 0.02 * length + 0.021, f"{name}: {node}'s capsule"


@needs_maps
def test_limits_within_the_hosts():
    for name, body in bodies().items():
        if body is None:
            continue
        for part in body[1][1:]:
            low, high = part["twist"]
            assert -math.pi <= low <= high <= math.pi, name
            assert all(0 <= swing <= math.pi for swing in part["swing"]), name


@needs_maps
def test_the_marine():
    """the stock body's shape, the one the others are made like"""
    found = profiles()
    nodes = found["characters\\marine\\marine"][0]
    mass, parts, _ = bodies()["characters\\marine\\marine"]
    named = {nodes[part["node"]][0]: part for part in parts}
    assert mass == pytest.approx(80.0)
    assert named["bip01 l calf"]["shape"] == "capsule" and named["bip01 l thigh"]["shape"] == "capsule"
    # a knee bends about z, far, and hardly twists
    assert named["bip01 l calf"]["swing"][1] == pytest.approx(math.radians(70), abs=1e-4)
    assert named["bip01 l calf"]["twist"][1] == pytest.approx(math.radians(5), abs=1e-4)
    # the legs are heavier than the arms
    assert named["bip01 l thigh"]["mass"] > named["bip01 l upperarm"]["mass"]


@needs_maps
def test_breaks_are_their_limbs():
    """each break names a part of its body, and the limbs a grenade tears are the bodies' that tear"""
    found = profiles()
    built = bodies()
    torn = set()
    for name, lines in sections().items():
        for line in lines:
            if not line.startswith("break "):
                continue
            node, damage = line[len("break ") :].rsplit(":", 1)
            model = next(model for model in built if model.lower() == name)
            nodes = found[model][0]
            named = {nodes[part["node"]][0].lower(): part for part in built[model][1]}
            assert node.strip().lower() in named, f"{model}: {node} is no part"
            assert named[node.strip().lower()]["break"] == pytest.approx(float(damage)), f"{model}: {node}"
            # (a frag grenade, 120 at its centre, tears it by chance at most 1.25 over)
            assert 0 < float(damage) <= 120 / 0.75, f"{model}: {node}"
            torn.add(model)
    for model in ("characters\\marine\\marine", "characters\\elite\\elite", "characters\\grunt\\grunt"):
        assert model in torn, model
    for model in ("characters\\hunter\\hunter", "characters\\sentinel\\sentinel"):
        assert model not in torn and all(not part["break"] for part in built[model][1]), model


@needs_maps
def test_the_hunters_overrides():
    found = profiles()
    nodes = found["characters\\hunter\\hunter"][0]
    mass, parts, node_parts = bodies()["characters\\hunter\\hunter"]
    named = {nodes[part["node"]][0]: part for part in parts}
    assert mass == pytest.approx(1100.0)
    assert named["bip01 l forearm"]["swing"] == pytest.approx([math.radians(5), math.radians(50)], abs=1e-4)
    for index, (node, *_) in enumerate(nodes):
        if node.startswith("frame spike"):
            assert node not in named, node


def test_profiles_report():
    """(prints each body, for reading: pytest -s)"""
    if not maps():
        pytest.skip("needs the stock maps in assets/maps")
    found = profiles()
    for name, body in sorted(bodies().items()):
        if body is None:
            print(f"{name}: none")
            continue
        nodes = found[name][0]
        print(f"{name}: {len(body[1])} parts, {body[0]:g} kg")
        for part in body[1]:
            parent = nodes[body[1][part['parent']]['node']][0] if part["parent"] >= 0 else "-"
            print(
                f"    {nodes[part['node']][0]:<24} on {parent:<24} {part['shape']:<8} "
                f"{' '.join(f'{s:.3f}' for s in part['size'])}  {part['mass']:.1f} kg"
            )
