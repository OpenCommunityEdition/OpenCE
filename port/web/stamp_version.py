"""Writes the web site's version.json (tools/web_build.py): which build the
site is, for the page (its About box, the update prompt) and its service
worker (port/web/site/sw.js), which keeps each build's files together under
its version.

    python port/web/stamp_version.py --flavor release OUTPUT FILE...

The version is a hash of the site's files, so that any change is a new one;
the build number (HALO_BUILD_NUMBER, which tools/ci_build.py gives builds of
main), the commit (GITHUB_SHA, else git's) and the network version
(HALO_PORT_NETWORK_VERSION, port/linux/include/halo_port_limits.h: which
native builds a web build could play with) go with it.
"""

import argparse
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
LIMITS = ROOT / "port/linux/include/halo_port_limits.h"


def network_version() -> int:
    match = re.search(r"#define\s+HALO_PORT_NETWORK_VERSION\s+(\d+)", LIMITS.read_text(encoding="utf-8"))
    return int(match.group(1)) if match else 0


def commit() -> str:
    if os.environ.get("GITHUB_SHA"):
        return os.environ["GITHUB_SHA"]
    try:
        return subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, text=True,
                              check=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return ""


def site_name(output: Path, path: Path) -> str:
    """a file's path in the site (beside version.json), with / separators"""
    return Path(os.path.relpath(path, output.parent)).as_posix()


def stamp(output: Path, inputs: list, flavor: str) -> dict:
    digest = hashlib.sha256()
    # (what the service worker caches: the site's files but its dot files,
    # which only tell GitHub Pages how to serve it)
    names = sorted(name for name in (site_name(output, Path(path)) for path in inputs)
                   if not Path(name).name.startswith("."))
    for path in sorted(inputs, key=lambda path: site_name(output, Path(path))):
        digest.update(site_name(output, Path(path)).encode())
        digest.update(Path(path).read_bytes())
    number = os.environ.get("HALO_BUILD_NUMBER", "0")
    version = {
        "version": digest.hexdigest()[:16],
        "build": int(number) if number.isdigit() else 0,
        "flavor": flavor,
        "commit": commit(),
        "network_version": network_version(),
        "files": names,
    }
    output.write_text(json.dumps(version, indent=1) + "\n", encoding="utf-8")
    return version


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--flavor", choices=["debug", "release"], default="debug")
    parser.add_argument("output", type=Path)
    parser.add_argument("inputs", nargs="+")
    args = parser.parse_args()
    stamp(args.output, args.inputs, args.flavor)


if __name__ == "__main__":
    main()
