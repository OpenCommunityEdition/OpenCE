"""Exports the ai_brain mod's trained PyTorch models to models.aibw, the one file MENDICANT AI's
runtime reads in the game (port/linux/game/mendicant/mendicant_infer.c), on every platform.

Run it with the mod's Python (it needs torch, and the mod's policy.py for the per-enemy models);
it only reads the mod:

    <mod>/.venv/bin/python -I tools/mendicant/export_models.py --mod <mod> \\
        --out <data root>/mods/mendicant/tags/mendicant/models.aibw

The models are chosen as play.sh chooses them: data/frozen when both per-enemy weights are there
(covenant.pt, marines.pt), else the newest training run with both; the strategic models are the
best kept (<side>_strategy_best.pt), else the latest (<side>_strategy.pt). --covenant, --marines,
--covenant-strategy and --marines-strategy choose others. A side whose file is missing is left
out, and the game lets the drill decide for it, as brain.py did.

The game finds the file through the mods folder under the key ai.mendicant.weights
("mendicant\\models.aibw" by default): mods/<mod>/tags/mendicant/models.aibw, in a mod whose
mod.toml has no `maps =`.

The file (all little-endian):
  0   u32 magic 'AIBW'      4  u32 version (1)     8  u32 tensor count   12 u32 doctrine hash
  16  u32 decisions         20 u32 chains          24 u32 step choices   28 u32 data offset
  32  u32 CRC-32 of the data region                36..63 zero
  64  tensor table, 96 bytes each: name[64] (NUL padded), u32 type (0 f32, 1 f16), u32 dimensions,
      u32 sizes[4], u32 offset (from the file's start, 64-aligned), u32 bytes
  then the data. A tensor keeps PyTorch's layout: a Linear's weight is [out][in], row-major.
Names are "<model>.<PyTorch name>", the models strategy.covenant, strategy.marines,
squad.covenant and squad.marines. The strategic model's training-only heads (value_head,
enemy_step) are left out, and so is the AI Chief's model, which MENDICANT AI does not have.

The doctrine hash is FNV-1a over the doctrine's chains (name and side) and steps, the decisions
the strategic models were trained for; the game refuses a file made for another doctrine
(mendicant_doctrine_hash, from mendicant_doctrine.h).
"""
import argparse
import json
import os
import struct
import sys
import zlib

MAGIC, VERSION = 0x57424941, 1
HEADER, ENTRY = 64, 96
TRAINING_ONLY = ("value_head.", "enemy_step.")


def doctrine_hash(table):
    """FNV-1a over the chains (name and side) and the steps: the decisions the weights are for."""
    text = "|".join("%s:%s" % (c["name"], c["side"]) for c in table["chains"]) + "#" + "|".join(table["steps"])
    h = 0x811C9DC5
    for byte in text.encode():
        h = ((h ^ byte) * 0x01000193) & 0xFFFFFFFF
    return h


def play_models(mod):
    """play.sh's choice of models: frozen first, else the newest training run with both sides."""
    data = os.path.join(mod, "data")
    frozen = os.path.join(data, "frozen")
    def both(folder):
        paths = (os.path.join(folder, "covenant.pt"), os.path.join(folder, "marines.pt"))
        return all(os.path.exists(path) and os.path.getsize(path) > 0 for path in paths)
    latest = None
    if both(frozen):
        latest = frozen
    else:
        runs = sorted((os.path.join(data, d) for d in os.listdir(data) if d.startswith("train_")),
            key=os.path.getmtime, reverse=True) if os.path.isdir(data) else []
        latest = next((run for run in runs if both(run)), None)
    if latest is None:
        return {}
    def strategy(side):
        for name in ("%s_strategy_best.pt" % side, "%s_strategy.pt" % side):
            path = os.path.join(latest, name)
            if os.path.exists(path) and os.path.getsize(path) > 0:
                return path
        return None
    return {"covenant": os.path.join(latest, "covenant.pt"), "marines": os.path.join(latest, "marines.pt"),
        "covenant_strategy": strategy("covenant"), "marines_strategy": strategy("marines")}


def squad_tensors(policy, torch, path, fresh_stance):
    """A per-enemy model's tensors, through policy.load_state, as brain.py loads them. Weights
    saved before the stance model existed have none: brain.py then sampled stances from a fresh,
    untrained stance model; the game plays them without stances unless --fresh-stance."""
    state = torch.load(path, map_location="cpu", weights_only=True)
    has_stance = any(key.startswith("stance.") for key in state)
    brain = policy.Brain()
    policy.load_state(brain, state)
    out = []
    for key, value in brain.state_dict().items():
        if key.startswith("stance.") and not has_stance and not fresh_stance:
            continue
        out.append((key, value.detach().float().contiguous().numpy()))
    if not has_stance:
        print("%s: no stance model (%s)" % (path, "a fresh one exported" if fresh_stance else "played without stances"),
            file=sys.stderr)
    return out


def strategy_tensors(policy, torch, path):
    """A strategic model's tensors, without its training-only heads. One trained for another
    doctrine (another number of decisions) is refused: brain.py would have played an untrained
    one in its place."""
    state = torch.load(path, map_location="cpu", weights_only=True)
    expected = policy.Strategy().state_dict()
    for key, value in expected.items():
        if key.startswith(TRAINING_ONLY):
            continue
        if key not in state or tuple(state[key].shape) != tuple(value.shape):
            raise SystemExit("%s: not a strategic model for this doctrine (%s)" % (path, key))
    return [(key, value.detach().float().contiguous().numpy()) for key, value in state.items()
        if key in expected and not key.startswith(TRAINING_ONLY)]


def write(path, tensors, table, half):
    count = len(tensors)
    offset = (HEADER + ENTRY * count + 63) & ~63
    data_start = offset
    entries, blobs = [], []
    for name, array in tensors:
        assert len(name) < 64 and array.ndim <= 4, name
        blob = (array.astype("<f2") if half else array.astype("<f4")).tobytes()
        dims = list(array.shape) + [0] * (4 - array.ndim)
        entries.append(struct.pack("<64sII4III", name.encode(), 1 if half else 0, array.ndim, *dims, offset, len(blob)))
        pad = (-len(blob)) % 64
        blobs.append(blob + b"\0" * pad)
        offset += len(blob) + pad
    data = b"".join(blobs)
    steps = len(table["steps"]) + 1
    chains = len(table["chains"])
    header = struct.pack("<8I", MAGIC, VERSION, count, doctrine_hash(table), chains * 4 + steps, chains, steps,
        data_start) + struct.pack("<I", zlib.crc32(data) & 0xFFFFFFFF) + b"\0" * 28
    body = header + b"".join(entries)
    body += b"\0" * (data_start - len(body))
    folder = os.path.dirname(os.path.abspath(path))
    os.makedirs(folder, exist_ok=True)
    with open(path, "wb") as f:
        f.write(body + data)
    return len(body) + len(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--mod", required=True, help="the ai_brain mod's folder (policy.py, data/)")
    parser.add_argument("--out", default="models.aibw")
    parser.add_argument("--covenant", help="the Covenant's per-enemy weights (default: play.sh's)")
    parser.add_argument("--marines", help="the Marines' per-enemy weights (default: play.sh's)")
    parser.add_argument("--covenant-strategy", help="the Covenant's strategic model (default: play.sh's)")
    parser.add_argument("--marines-strategy", help="the Marines' strategic model (default: play.sh's)")
    parser.add_argument("--doctrine", help="doctrine_table.json (default: the mod's data/doctrine)")
    parser.add_argument("--half", action="store_true", help="store f16 (half the size; check the decisions after)")
    parser.add_argument("--fresh-stance", action="store_true",
        help="for weights with no stance model, export an untrained one, as brain.py played them")
    options = parser.parse_args()

    mod = os.path.abspath(options.mod)
    sys.path.insert(0, mod)
    import numpy  # noqa: F401 (policy needs it)
    import torch
    import policy
    torch.manual_seed(0)  # a fresh stance model (--fresh-stance) the same in every export

    chosen = play_models(mod)
    files = {"covenant": options.covenant or chosen.get("covenant"),
        "marines": options.marines or chosen.get("marines"),
        "covenant_strategy": options.covenant_strategy or chosen.get("covenant_strategy"),
        "marines_strategy": options.marines_strategy or chosen.get("marines_strategy")}
    doctrine = options.doctrine or os.path.join(mod, "data", "doctrine", "doctrine_table.json")
    table = json.load(open(doctrine))
    if len(table["chains"]) * 4 + len(table["steps"]) + 1 != policy.DECISIONS:
        raise SystemExit("%s is not the doctrine policy.py was loaded with" % doctrine)

    tensors = []
    for side in ("covenant", "marines"):
        path = files[side + "_strategy"]
        if path and os.path.exists(path):
            tensors += [("strategy.%s.%s" % (side, key), value) for key, value in strategy_tensors(policy, torch, path)]
            print("strategy.%s: %s" % (side, path))
        else:
            print("strategy.%s: none (the drill decides)" % side)
    for side in ("covenant", "marines"):
        path = files[side]
        if path and os.path.exists(path):
            tensors += [("squad.%s.%s" % (side, key), value)
                for key, value in squad_tensors(policy, torch, path, options.fresh_stance)]
            print("squad.%s: %s" % (side, path))
        else:
            print("squad.%s: none (stays put)" % side)
    if not tensors:
        raise SystemExit("no models found in %s" % mod)
    size = write(options.out, tensors, table, options.half)
    print("%s: %d tensors, %d parameters, %d bytes, doctrine %08x" % (options.out, len(tensors),
        sum(a.size for _, a in tensors), size, doctrine_hash(table)))


if __name__ == "__main__":
    main()
