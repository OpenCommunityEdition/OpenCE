#!/usr/bin/env python3
"""Check that a build with MENDICANT AI off plays a campaign level as a stock
build does, as far as the stock build plays it the same way twice.

Each copy runs the level alone, headless, one tick a frame (debug.fixed_step)
from the game's fixed random seed, and writes its whole game state (a core
save, core_save_name) at fixed game seconds. The reference (a build without
MENDICANT AI, or any build to compare against) runs --runs times, and so does
the candidate; what the reference's own runs disagree on at a second is its
noise there, and only the bytes outside every run's noise count against the
candidate. Each differing byte is put in the game state's data array that
holds it (found by their 'd@t@' headers in the core save), with the element
and the offset in it, so a difference can be told from padding, render caches
and the like.

    python3 tools/mendicant/ai_off_parity.py --reference <stock build>/build/linux/halo \\
        --candidate build/linux/halo --maps <the game's maps folder>

Every MENDICANT AI and ai_brain environment variable is cleared for every
copy, so the candidate runs with the AI off (its default). Exit status: 0 when
the candidate differs from the reference nowhere outside the noise, 1 when it
does, 2 when a copy could not be run.

Each copy runs in a session of its own, which is ended if it outlives
--timeout; nothing is written outside --out.
"""

import argparse
import collections
import json
import os
import signal
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent

# a data array's header (source/memory/data.h): the name, 32 characters and
# a terminator, then the maximum count and the element size, and at 40 the
# signature 'd@t@' (little-endian); its elements follow the 56-byte header
DATA_SIGNATURE = b"@t@d"
DATA_NAME_LENGTH = 32
DATA_SIGNATURE_OFFSET = 40
DATA_HEADER_SIZE = 56
CHUNK = 4096


def data_arrays(state: bytes) -> list:
    """(start, end, name, element size) of each data array in a core save"""
    arrays = []
    at = state.find(DATA_SIGNATURE)
    while at >= 0:
        header = at - DATA_SIGNATURE_OFFSET
        if header >= 0:
            name = state[header:header + DATA_NAME_LENGTH].split(b"\0")[0].decode("latin-1")
            maximum, size = struct.unpack_from("<hh", state, header + DATA_NAME_LENGTH)
            if name and maximum > 0 and size > 0:
                arrays.append((header, header + DATA_HEADER_SIZE + maximum * size, name, size))
        at = state.find(DATA_SIGNATURE, at + 4)
    arrays.sort()
    return arrays


def differing_bytes(a: bytes, b: bytes) -> set:
    """the offsets at which two core saves differ (a length difference: every
    byte past the shorter)"""
    found = set()
    length = min(len(a), len(b))
    for chunk in range(0, length, CHUNK):
        end = min(chunk + CHUNK, length)
        if a[chunk:end] != b[chunk:end]:
            found.update(offset for offset in range(chunk, end) if a[offset] != b[offset])
    found.update(range(length, max(len(a), len(b))))
    return found


def attribute(offsets, arrays: list) -> dict:
    """{array name: {"bytes": n, "fields": {element offset: n}, "elements": n}}
    ("(other)": outside every data array, its fields the 4 KiB pages)"""
    report = {}
    starts = [array[0] for array in arrays]
    elements = collections.defaultdict(set)
    for offset in sorted(offsets):
        # the last array starting at or before the offset, if it holds it
        low, high = 0, len(starts)
        while low < high:
            middle = (low + high) // 2
            if starts[middle] <= offset:
                low = middle + 1
            else:
                high = middle
        array = arrays[low - 1] if low else None
        if array and offset < array[1]:
            name = array[2]
            relative = offset - array[0] - DATA_HEADER_SIZE
            field = "header" if relative < 0 else "0x%x" % (relative % array[3])
            if relative >= 0:
                elements[name].add(relative // array[3])
        else:
            name, field = "(other)", "page 0x%x" % (offset // CHUNK * CHUNK)
        entry = report.setdefault(name, {"bytes": 0, "fields": collections.Counter()})
        entry["bytes"] += 1
        entry["fields"][field] += 1
    for name, entry in report.items():
        entry["elements"] = len(elements[name])
        entry["fields"] = dict(entry["fields"].most_common(8))
    return dict(sorted(report.items(), key=lambda item: -item[1]["bytes"]))


def run_copy(binary: Path, directory: Path, maps: Path, level: str, seconds: list, timeout: float) -> str:
    """one copy: the level from its start, core saves s<second>; None when it
    ran, else why not"""
    data = directory / "data"
    for folder in (data, directory / "saves", directory / "logs"):
        folder.mkdir(parents=True, exist_ok=True)
    (data / "maps").symlink_to(maps.resolve())
    (data / "init.txt").write_text("map_name %s\n" % level)
    cues = ["%d core_save_name s%d" % (second, second) for second in seconds]
    cues.append("%d quit" % (seconds[-1] + 3))
    environment = {
        name: value for name, value in os.environ.items()
        if not name.startswith(("HALO_", "AI_BRAIN_"))
    }
    environment.update({
        "HALO_NULL_RENDERER": "1", "HALO_HIDDEN_WINDOW": "1", "HALO_NO_AUDIO": "1",
        "HALO_FIXED_STEP": "1", "HALO_NETWORK_TEST": "log",
        "HALO_NETWORK_TEST_CUES": "|".join(cues),
        "HALO_CONFIG_FILE": str(directory / "config.toml"),
        "HALO_DATA_ROOT": str(data), "HALO_SAVE_ROOT": str(directory / "saves"),
        "HALO_LOG_ROOT": str(directory / "logs"),
    })
    with open(directory / "out.txt", "wb") as output:
        process = subprocess.Popen([str(binary.resolve())], cwd=directory, env=environment,
            stdout=output, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            return "still running after %.0f s" % timeout
    missing = [second for second in seconds if not (data / "core" / ("s%d" % second)).is_file()]
    if missing:
        return "no core save at second(s) %s (exit status %d)" % (missing, process.returncode)
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--reference", type=Path, required=True, help="the stock build's executable")
    parser.add_argument("--candidate", type=Path, default=ROOT / "build/linux/halo")
    parser.add_argument("--maps", type=Path, required=True, help="the game's maps folder")
    parser.add_argument("--level", default="levels\\b30\\b30")
    parser.add_argument("--seconds", default="5,10,15,20,30,45,60",
        help="game seconds to compare at (from the first tick the player has a unit)")
    parser.add_argument("--runs", type=int, default=2, help="copies of each build (2 or more find the noise)")
    parser.add_argument("--timeout", type=float, default=600.0, help="seconds a copy may run")
    parser.add_argument("--out", type=Path, default=ROOT / "build/ai_off_parity")
    parser.add_argument("--keep", action="store_true", help="keep the core saves")
    arguments = parser.parse_args()
    arguments.out = arguments.out.resolve()
    seconds = sorted({int(second) for second in arguments.seconds.split(",")})

    copies = {"reference": [], "candidate": []}
    for run in range(arguments.runs):
        for build in ("reference", "candidate"):
            directory = arguments.out / ("%s%d" % (build, run + 1))
            if directory.exists():
                subprocess.run(["rm", "-rf", "--", str(directory)], check=True)
            started = time.monotonic()
            failure = run_copy(getattr(arguments, build), directory, arguments.maps, arguments.level,
                seconds, arguments.timeout)
            print("%s %d: %s (%.0f s)" % (build, run + 1, failure or "ran", time.monotonic() - started),
                flush=True)
            if failure:
                return 2
            copies[build].append(directory / "data" / "core")

    report = {"level": arguments.level, "runs": arguments.runs, "seconds": {}}
    worst = 0
    for second in seconds:
        name = "s%d" % second
        states = {build: [(folder / name).read_bytes() for folder in folders] for build, folders in copies.items()}
        arrays = data_arrays(states["reference"][0])
        noise = {}
        noise_union = set()
        for build in ("reference", "candidate"):
            offsets = set()
            for other in states[build][1:]:
                offsets |= differing_bytes(states[build][0], other)
            noise[build] = attribute(offsets, arrays)
            noise_union |= offsets
        against = set()
        for candidate in states["candidate"]:
            for reference in states["reference"]:
                against |= differing_bytes(reference, candidate)
        beyond = against - noise_union
        report["seconds"][second] = {
            "reference_noise": noise["reference"],
            "candidate_noise": noise["candidate"],
            "candidate_vs_reference": attribute(against, arrays),
            "beyond_noise": attribute(beyond, arrays),
        }
        worst = max(worst, len(beyond))
        print("second %d: reference's own runs differ in %d bytes, the candidate's in %d; the candidate "
            "against the reference in %d, %d of them outside the noise"
            % (second, sum(entry["bytes"] for entry in noise["reference"].values()),
                sum(entry["bytes"] for entry in noise["candidate"].values()),
                len(against), len(beyond)))
        for label, part in (("  reference noise", noise["reference"]), ("  beyond noise", report["seconds"][second]["beyond_noise"])):
            for array, entry in list(part.items())[:6]:
                print("%s: %s %d bytes in %d elements, fields %s"
                    % (label, array, entry["bytes"], entry["elements"], entry["fields"]))
    arguments.out.mkdir(parents=True, exist_ok=True)
    (arguments.out / "report.json").write_text(json.dumps(report, indent=1))
    print("report: %s" % (arguments.out / "report.json"))
    if not arguments.keep:
        for folders in copies.values():
            for folder in folders:
                for second in seconds:
                    (folder / ("s%d" % second)).unlink(missing_ok=True)
    return 1 if worst else 0


if __name__ == "__main__":
    sys.exit(main())
