"""Portable file staging and SDL bootstrap for the Web Ninja graph."""

import argparse
from pathlib import Path
import shutil
import subprocess
import sys


def tool_command(tool):
    path = Path(tool)
    # SDK launchers on Windows are batch files; call their Python entry point
    # directly so paths and arguments need no second command-shell quoting.
    if path.suffix == ".bat" and path.with_suffix(".py").is_file():
        path = path.with_suffix(".py")
    return [sys.executable, str(path)] if path.suffix == ".py" else [str(path)]


def copy(source, destination):
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)


def sdl(emcc, port, stamp, license_path, tag):
    stamp = Path(stamp)
    stamp.parent.mkdir(parents=True, exist_ok=True)
    source, obj = Path(str(stamp) + ".c"), Path(str(stamp) + ".o")
    try:
        source.write_text("#include <SDL3/SDL.h>\n", encoding="ascii")
        subprocess.run([*tool_command(emcc), f"--use-port={port}", "-pthread", "-c", str(source),
                        "-o", str(obj)], check=True)
        config = Path(emcc).with_name("em-config" + Path(emcc).suffix)
        cache = subprocess.check_output([*tool_command(config), "CACHE"], text=True).strip()
        copy(Path(cache) / "ports" / "halo_sdl3" / f"SDL-{tag}" / "LICENSE.txt", license_path)
        stamp.touch()
    finally:
        source.unlink(missing_ok=True)
        obj.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    stage = commands.add_parser("copy")
    stage.add_argument("source")
    stage.add_argument("destination")
    bootstrap = commands.add_parser("sdl")
    for name in ("emcc", "port", "stamp", "license", "tag"):
        bootstrap.add_argument("--" + name, required=True)
    args = parser.parse_args()
    if args.command == "copy":
        copy(args.source, args.destination)
    else:
        sdl(args.emcc, args.port, args.stamp, args.license, args.tag)


if __name__ == "__main__":
    main()
