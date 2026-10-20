"""The stock bipeds' ragdolls on Jolt, as the maps in assets/maps make them: each biped's body (built by the sources,
tools/test_ragdoll_profiles.py) in its model's default pose, given to the host's physics bridge
(port/macos/host/physics) as the guest gives it (port/linux/game/physics_jolt.c), with the frag grenade's push and the
warthog's hull read from the same maps. The program (port/macos/tests/host_ragdoll_profiles_test.c) runs the batches
natively and checks that:

- every body falls onto the ground and comes to rest there, whole, near where it fell;
- a frag grenade's push (its instantaneous acceleration times the biped's acceleration scale, lifted as damage.c lifts
  it, half of it on a killing hit and all of it on a corpse) moves each body at the speed the game gives a biped;
- a warthog's hull, driven at the ragdoll as a kinematic stand-in, shoves it along without passing through it."""

import functools
import math
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from test_ragdoll_profiles import (BSP_VERTICES, COLLISION_MODEL_NODES, COLLISION_NODE_BSPS,  # noqa: E402
                                   COLLISION_NODE_SIZE, MODEL_NODE_SIZE, MODEL_NODES, OBJECT_COLLISION_MODEL,
                                   OBJECT_MODEL, TAG_BASE, VERTEX_SIZE, bodies, maps, needs_maps, profiles,
                                   tags_of)

PHYSICS_LIBRARY = ROOT / "build/macos/libhalo_physics.a"
PROGRAM = "host_ragdoll_profiles_test"
# offsets in the tags: an object's acceleration scale, a damage effect's instantaneous acceleration, a model node's
# default rotation, a collision bsp's size
OBJECT_ACCELERATION_SCALE = 32
DAMAGE_INSTANTANEOUS_ACCELERATION = 500
MODEL_NODE_ROTATION = 52
COLLISION_BSP_SIZE = 96

FRAG = "weapons\\frag grenade\\explosion"
WARTHOG = "vehicles\\warthog\\warthog"
# the bipeds the pushes are measured on, each its model's name too
PUSHED = ("characters\\marine\\marine", "characters\\elite\\elite", "characters\\grunt\\grunt",
          "characters\\hunter\\hunter")


class Map:
    def __init__(self, path):
        self.tags = tags_of(path)
        index_address, _, _, count = self.at(TAG_BASE, "IIII")
        self.handles, self.named = {}, {}
        for index in range(count):
            group, _, _, handle, name, address, _, _ = self.at(index_address + 32 * index, "IIIIIIII")
            entry = (struct.pack(">I", group).decode("latin-1"), self.name_at(name), address)
            self.handles[handle & 0xFFFF] = entry
            self.named[entry[:2]] = address

    def at(self, address, layout):
        return struct.unpack_from("<" + layout, self.tags, address - TAG_BASE)

    def name_at(self, address, length=None):
        start = address - TAG_BASE
        end = start + length if length else self.tags.index(b"\0", start)
        return self.tags[start:end].split(b"\0")[0].decode("latin-1")

    def reference(self, address):
        return self.handles.get(self.at(address + 12, "I")[0] & 0xFFFF)


@functools.lru_cache(maxsize=None)
def stock():
    """{model name: (acceleration scale, [(parent, translation, game rotation)])}, the frag's acceleration, and the
    warthog's hull (its first collision node's box: centre and half extents)"""
    models, frag, hull = {}, None, None
    for path in maps():
        tags = Map(path)
        for (group, name), address in tags.named.items():
            if group == "bipd":
                model = tags.reference(address + OBJECT_MODEL)
                if not model or model[0] != "mode" or model[1] in models:
                    continue
                node_count, node_address = tags.at(model[2] + MODEL_NODES, "iI")
                nodes = []
                for node in range(node_count):
                    element = node_address + MODEL_NODE_SIZE * node
                    nodes.append((tags.at(element + 36, "h")[0], tags.at(element + 40, "fff"),
                                  tags.at(element + MODEL_NODE_ROTATION, "ffff")))
                models[model[1]] = (tags.at(address + OBJECT_ACCELERATION_SCALE, "f")[0], nodes)
            elif (group, name) == ("jpt!", FRAG) and frag is None:
                frag = tags.at(address + DAMAGE_INSTANTANEOUS_ACCELERATION, "f")[0]
            elif (group, name) == ("vehi", WARTHOG) and hull is None:
                collision = tags.reference(address + OBJECT_COLLISION_MODEL)
                _, piece_address = tags.at(collision[2] + COLLISION_MODEL_NODES, "iI")
                bsp_count, bsp_address = tags.at(piece_address + COLLISION_NODE_BSPS, "iI")
                points = []
                for bsp in range(bsp_count):
                    vertex_count, vertex_address = tags.at(bsp_address + COLLISION_BSP_SIZE * bsp + BSP_VERTICES, "iI")
                    points += [tags.at(vertex_address + VERTEX_SIZE * v, "fff") for v in range(vertex_count)]
                low = [min(p[axis] for p in points) for axis in range(3)]
                high = [max(p[axis] for p in points) for axis in range(3)]
                hull = ([(l + h) / 2 for l, h in zip(low, high)], [(h - l) / 2 for l, h in zip(low, high)])
    return models, frag, hull


def multiply(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


def rotate(q, v):
    x, y, z, _ = multiply(multiply(q, (*v, 0.0)), (-q[0], -q[1], -q[2], q[3]))
    return (x, y, z)


def default_frames(nodes):
    """each node's frame in the object's, as the game's node matrices are in the default pose: a node's own rotation
    the conjugate of the tag's (matrix4x3_rotation_from_quaternion), after its parent's"""
    frames = []
    for parent, translation, (i, j, k, w) in nodes:
        length = math.sqrt(i * i + j * j + k * k + w * w) or 1.0
        local = (-i / length, -j / length, -k / length, w / length)
        if parent < 0:
            frames.append((translation, local))
            continue
        position, rotation = frames[parent]
        offset = rotate(rotation, translation)
        frames.append((tuple(p + o for p, o in zip(position, offset)), multiply(rotation, local)))
    return frames


def define(text, name):
    return float(re.search(rf"#define {name} ([0-9.]+)f?", text)[1])


def floats(values):
    return ", ".join(f"{v!r}f" for v in values)


def word(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def profiles_include():
    models, frag, hull = stock()
    jolt = (ROOT / "port/linux/game/physics_jolt.c").read_text(encoding="utf-8")
    ragdolls = (ROOT / "port/linux/game/ragdolls.h").read_text(encoding="utf-8")
    entries = []
    for name, body in sorted(bodies().items()):
        if body is None:
            continue
        mass, parts, _ = body
        scale, nodes = models[name]
        frames = default_frames(nodes)
        # (each part's words as physics_jolt_part_words writes them)
        words, poses = [], []
        for part in parts:
            words += [part["parent"] & 0xFFFFFFFF, {"sphere": 0, "capsule": 1, "box": 2}[part["shape"]]]
            words += [word(v) for v in (*part["size"], *part["center"], 0, 0, 0, 1, part["mass"], 1, 0, 0, 0, 0, 1,
                                        *part["twist"], *part["swing"])]
            position, rotation = frames[part["node"]]
            poses += [*position, *rotation]
        entries.append(f"\t{{\"{name.replace(chr(92), '/')}\", {len(parts)}, {mass!r}f, {scale!r}f,\n"
                       f"\t\t{{{', '.join(f'0x{w:08x}u' for w in words)}}},\n\t\t{{{floats(poses)}}}}},")
    return "\n".join([
        f"#define RAGDOLL_FRICTION {define(jolt, 'PHYSICS_JOLT_RAGDOLL_FRICTION')!r}f",
        f"#define RAGDOLL_RESTITUTION {define(jolt, 'PHYSICS_JOLT_RAGDOLL_RESTITUTION')!r}f",
        f"#define STAND_IN_FRICTION {define(jolt, 'PHYSICS_JOLT_STAND_IN_FRICTION')!r}f",
        f"#define STAND_IN_RESTITUTION {define(jolt, 'PHYSICS_JOLT_STAND_IN_RESTITUTION')!r}f",
        f"#define PUSH_LIFT {define(ragdolls, 'RAGDOLL_PUSH_LIFT')!r}f",
        f"#define FRAG_ACCELERATION {frag!r}f",
        f"static const float warthog_center[3] = {{{floats(hull[0])}}};",
        f"static const float warthog_half_extents[3] = {{{floats(hull[1])}}};",
        "static const char *const pushed[] = {" + ", ".join(f'"{n.replace(chr(92), "/")}"' for n in PUSHED) + "};",
        "static const struct profile profiles[] = {", *entries, "};", ""])


@pytest.fixture(scope="module")
def program(tmp_path_factory):
    built = subprocess.run(["ninja", str(PHYSICS_LIBRARY.relative_to(ROOT))], cwd=ROOT, capture_output=True, text=True)
    assert built.returncode == 0, built.stdout + built.stderr
    folder = tmp_path_factory.mktemp("ragdoll_physics")
    (folder / "ragdoll_profiles.inc").write_text(profiles_include(), encoding="utf-8")
    build = subprocess.run([
        os.environ.get("CC", "cc"), "-std=gnu11", "-O1", "-Wall", "-Werror", "-mmacosx-version-min=26.0",
        "-Iport/macos/host", "-Iport/macos/include", "-I", str(folder), f"port/macos/tests/{PROGRAM}.c",
        str(PHYSICS_LIBRARY), "-lc++", "-o", str(folder / PROGRAM),
    ], cwd=ROOT, capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    return folder / PROGRAM


@needs_maps
def test_the_stock_values():
    """(what the cases rely on: the maps' values as the stock game has them)"""
    models, frag, hull = stock()
    assert frag == pytest.approx(4.0)
    assert models["characters\\marine\\marine"][0] == pytest.approx(1.0)
    assert models["characters\\hunter\\hunter"][0] == pytest.approx(0.5)
    assert hull[1] == pytest.approx([0.96, 0.45, 0.29], abs=0.01)


@needs_maps
def test_the_marine_stands():
    """the default pose's frames, as the game makes them: the head above the pelvis, above the feet"""
    nodes = stock()[0]["characters\\marine\\marine"][1]
    names = [node[0] for node in profiles()["characters\\marine\\marine"][0]]
    frames = dict(zip(names, default_frames(nodes)))
    head, pelvis, foot = (frames[f"bip01 {name}"][0][2] for name in ("head", "pelvis", "l foot"))
    assert head > pelvis + 0.1 > foot + 0.1


CASES = ["every_body_comes_to_rest", "a_frag_grenade_pushes_as_the_game_does", "a_warthog_shoves_a_marine"]


@needs_maps
@pytest.mark.parametrize("case", CASES)
def test_physics(program, case):
    result = subprocess.run([str(program), case], capture_output=True, text=True, timeout=300)
    print(result.stdout)
    assert (result.returncode, result.stdout.strip().splitlines()[-1:]) == (0, ["ok"]), result.stdout + result.stderr
