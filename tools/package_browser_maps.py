#!/usr/bin/env python3
"""Extract supported Xbox maps into a static, chunked download directory."""
import argparse
import hashlib
import json
from pathlib import Path

from serve_browser import disc_maps

CHUNK_BYTES = 48 * 1024 * 1024


def package(image, output):
    maps = disc_maps(image)
    output.mkdir(parents=True, exist_ok=True)
    if any(output.iterdir()):
        raise ValueError("Output directory must be empty")
    (output / "chunks").mkdir()
    files = []
    with image.open("rb") as disc:
        for path, (offset, size) in sorted(maps.items()):
            name = path.removeprefix("/assets/")
            disc.seek(offset)
            remaining, chunks = size, []
            digest = hashlib.sha256()
            while remaining:
                block = disc.read(min(CHUNK_BYTES, remaining))
                if not block:
                    raise ValueError(f"Truncated disc: {name}")
                relative = f"chunks/{Path(name).name}.part{len(chunks):03d}"
                (output / relative).write_bytes(block)
                chunks.append({"path": relative, "size": len(block)})
                digest.update(block)
                remaining -= len(block)
            files.append({"name": name, "size": size, "sha256": digest.hexdigest(), "chunks": chunks})
            print(f"Packaged {name}: {size:,} bytes in {len(chunks)} chunks", flush=True)
    (output / "manifest.json").write_text(json.dumps({"version": 2, "files": files}, indent=2) + "\n")
    print(f"Wrote {len(files)} maps, {sum(f['size'] for f in files):,} bytes to {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    package(args.image.expanduser().resolve(), args.output.resolve())


if __name__ == "__main__":
    main()
