"""The edit-build-test loop's timings, measured the same way each time
(the Rust tools track measures before and after each of its changes).

Cold builds go to their own target folder under --scratch, so the
checkout's build/ and engine/target are left as they are. The steps:

  build_dev_cold      cargo build --workspace, from nothing
  build_dev_touch     the same after touching the sim (an edit's rebuild)
  build_tests         cargo test --workspace --no-run, after build_dev_cold
                      (in --profile, test or test-opt)
  build_release_cold  the library the app links, release, from nothing
  suites              each test binary run on its own, one after another
  app                 tools/engine_build.py's app, then devgulch's load
                      and its release frame times (--benchmark)

Run inside nix develop. The timings go to --out as JSON and print as a
table; --only picks steps."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import time
from collections import Counter
from dataclasses import dataclass, replace
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ENGINE = ROOT / "engine"
STEPS = ("build_dev_cold", "build_dev_touch", "build_tests", "build_release_cold", "suites", "app")


@dataclass(frozen=True)
class Suite:
    """A test binary cargo built."""

    name: str
    executable: Path
    package: Path


def test_binaries(lines: list[str]) -> list[Suite]:
    """The test binaries in cargo's --message-format=json `lines`."""
    suites = []
    for line in lines:
        try:
            message = json.loads(line)
        except json.JSONDecodeError:
            continue
        # (a binary cargo builds for the integration tests to run is not one)
        if message.get("reason") != "compiler-artifact" or not message.get("executable"):
            continue
        if not message.get("profile", {}).get("test", True):
            continue
        target = message["target"]
        kind = target["kind"][0]
        name = target["name"] if kind == "test" else f"{target['name']} ({kind})"
        suites.append(Suite(name, Path(message["executable"]), Path(message["manifest_path"]).parent))
    # (two crates' suites of one name would share a key in the timings)
    counts = Counter(suite.name for suite in suites)
    return [
        replace(suite, name=f"{suite.name} ({suite.package.name})") if counts[suite.name] > 1 else suite
        for suite in suites
    ]


def table(timings: dict, slowest: int = 10) -> str:
    """`timings` as a markdown table, with the `slowest` suites."""
    rows = ["| step | time |", "|---|---|"]
    rows += [f"| {step} | {seconds:.1f} s |" for step, seconds in timings.get("steps", {}).items()]
    suites = timings.get("suites", {})
    if suites:
        rows.append(f"| test suites ({len(suites)}) | {sum(s['seconds'] for s in suites.values()):.1f} s |")
    for name, value in timings.get("benchmark", {}).items():
        rows.append(f"| devgulch {name} | {value} |")
    if suites:
        rows += ["", "| slowest suites | time | |", "|---|---|---|"]
        ranked = sorted(suites.items(), key=lambda item: -item[1]["seconds"])[:slowest]
        rows += [f"| {name} | {s['seconds']:.1f} s | {'passed' if s['passed'] else 'failed'} |" for name, s in ranked]
    return "\n".join(rows)


def timed(command: list[str], cwd: Path, environment: dict[str, str]) -> float:
    start = time.monotonic()
    subprocess.run(command, cwd=cwd, env=environment, check=True)
    return time.monotonic() - start


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--scratch", type=Path, required=True, help="where the cold builds' target folders go")
    parser.add_argument("--out", type=Path, default=ROOT / "build" / "engine" / "timings.json")
    parser.add_argument("--only", help="comma-separated steps, of " + ", ".join(STEPS))
    parser.add_argument("--benchmark-seconds", type=float, default=20)
    parser.add_argument("--profile", default="test", help="the cargo profile the tests are built in")
    arguments = parser.parse_args()
    steps = arguments.only.split(",") if arguments.only else list(STEPS)
    unknown = set(steps) - set(STEPS)
    if unknown:
        parser.error(f"no such steps: {', '.join(sorted(unknown))}")

    timings: dict = json.loads(arguments.out.read_text()) if arguments.out.exists() else {}
    timings.setdefault("steps", {})
    environment = dict(os.environ)
    dev = arguments.scratch / "target-dev"
    release = arguments.scratch / "target-release"

    def record(step: str, command: list[str], target: Path, cold: bool) -> None:
        if cold:
            shutil.rmtree(target, ignore_errors=True)
        seconds = timed(command, ENGINE, environment | {"CARGO_TARGET_DIR": str(target)})
        timings["steps"][step] = seconds
        print(f"{step}: {seconds:.1f} s", flush=True)

    if "build_dev_cold" in steps:
        record("build_dev_cold", ["cargo", "build", "--locked", "--workspace"], dev, cold=True)
    if "build_dev_touch" in steps:
        (ENGINE / "crates" / "sim" / "src" / "lib.rs").touch()
        record("build_dev_touch", ["cargo", "build", "--locked", "--workspace"], dev, cold=False)
    if "build_tests" in steps:
        record(
            "build_tests",
            ["cargo", "test", "--locked", "--workspace", "--profile", arguments.profile, "--no-run"],
            dev,
            cold=False,
        )
    if "build_release_cold" in steps:
        record(
            "build_release_cold",
            ["cargo", "build", "--locked", "--release", "-p", "opence_ffi", "--lib"],
            release,
            cold=True,
        )
    if "suites" in steps:
        listing = subprocess.run(
            [
                "cargo",
                "test",
                "--locked",
                "--workspace",
                "--profile",
                arguments.profile,
                "--no-run",
                "--message-format=json",
            ],
            cwd=ENGINE,
            env=environment | {"CARGO_TARGET_DIR": str(dev)},
            capture_output=True,
            text=True,
            check=True,
        )
        timings["suites"] = {}
        for suite in test_binaries(listing.stdout.splitlines()):
            start = time.monotonic()
            run = subprocess.run(
                [str(suite.executable)],
                cwd=suite.package,
                env=environment | {"CARGO_MANIFEST_DIR": str(suite.package)},
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
            )
            seconds = time.monotonic() - start
            timings["suites"][suite.name] = {"seconds": seconds, "passed": run.returncode == 0}
            print(f"  {suite.name}: {seconds:.1f} s{'' if run.returncode == 0 else ' (failed)'}", flush=True)
    if "app" in steps:
        out = ROOT / "build" / "engine"
        start = time.monotonic()
        subprocess.run(["python3", str(ROOT / "tools" / "engine_build.py"), "--out", str(out)], check=True)
        timings["steps"]["app_build"] = time.monotonic() - start
        executable = out / "OpenCE Engine.app" / "Contents" / "MacOS" / "OpenCEEngine"
        devgulch = ROOT / "assets" / "maps" / "devgulch.map"
        cache = out / "cache"
        seconds = arguments.benchmark_seconds
        start = time.monotonic()
        result = subprocess.run(
            [str(executable), "--benchmark", str(seconds), "--map", str(devgulch), "--cache", str(cache)],
            capture_output=True,
            text=True,
            check=True,
        )
        wall = time.monotonic() - start
        report = json.loads(result.stdout.strip().splitlines()[-1])
        timings["benchmark"] = {
            "p50": f"{report['p50_ms']:.2f} ms",
            "p99": f"{report['p99_ms']:.2f} ms",
            "gpu p99": f"{report['gpu_p99_ms']:.2f} ms",
            "start and load (cached)": f"{wall - seconds:.1f} s",
        }

    arguments.out.parent.mkdir(parents=True, exist_ok=True)
    arguments.out.write_text(json.dumps(timings, indent=2) + "\n")
    print(table(timings))


if __name__ == "__main__":
    main()
