#!/usr/bin/env python3
"""Synthetic XDVDFS (Xbox disc image) generator for testing OpenCE Setup.

Makes small images with a fake maps/ folder (files with valid-looking Xbox
cache headers, random bodies), decoy files, and deliberately broken images.
Contains no Halo data. Usage: make_xiso.py OUTPUT_DIR
Writes OUTPUT_DIR/<name>.xiso, OUTPUT_DIR/ref/maps (what a good image holds)
and OUTPUT_DIR/expect.txt ("<name> <ok|refuse|either>" per line).
"""
import os
import random
import struct
import sys

SECTOR = 2048
MAGIC = b"MICROSOFT*XBOX*MEDIA"
MAP_NAMES = ["ui", "bloodgulch", "a10", "a30", "b30", "d40", "chillout"]


def cache_file(name, size, version=5, head=b"daeh", foot=b"toof", seed=0):
    rng = random.Random(name + str(seed))
    header = bytearray(0x800)
    header[0:4] = head
    struct.pack_into("<ii", header, 4, version, size)
    header[0x20:0x20 + len(name)] = name.encode()
    header[0x40:0x48] = b"01.10.12"
    header[0x7FC:0x800] = foot
    body = bytes(rng.getrandbits(8) for _ in range(min(size, 65536) - 0x800))
    body = (body * (size // len(body) + 1))[: size - 0x800]
    return bytes(header) + body


def good_maps():
    files = {}
    for n in MAP_NAMES:
        files[n + ".map"] = cache_file(n, 3 * 1024 * 1024 + 123 if n == "bloodgulch" else 20000 + len(n) * 777)
    files["loading.tga"] = b"TGA decoy " * 100
    files["readme.txt"] = b"decoy\n"
    return files


class Node:
    def __init__(self, name, data=None, children=None):
        self.name, self.data, self.children = name, data, children


def table_for(entries):
    """entries: list of (name, sector, size, attr). Returns a directory table:
    a balanced binary tree (links in 4-byte units), the root at offset 0."""
    entries = sorted(entries, key=lambda e: e[0].lower())

    def esize(e):
        return (14 + len(e[0].encode()) + 3) & ~3

    order = []  # preorder: (entry, left child index or None, right ...)
    offsets = {}

    def visit(lo, hi):
        if lo >= hi:
            return None
        mid = (lo + hi) // 2
        offsets[mid] = None
        order.append(mid)
        left = visit(lo, mid)
        right = visit(mid + 1, hi)
        links[mid] = (left, right)
        return mid

    links = {}
    visit(0, len(entries))
    pos = 0
    for i in order:
        offsets[i] = pos
        pos += esize(entries[i])
    buf = bytearray()
    for i in order:
        left, right = links[i]
        lw = offsets[left] // 4 if left is not None else 0
        rw = offsets[right] // 4 if right is not None else 0
        name, sector, size, attr = entries[i]
        e = struct.pack("<HHIIBB", lw, rw, sector, size, attr, len(name.encode())) + name.encode()
        e += b"\xff" * (esize(entries[i]) - len(e))
        buf += e
    return bytes(buf)


def pad(data):
    n = (-len(data)) % SECTOR
    return data + b"\xff" * n


def build_image(root_files, partition=0, tweak=None):
    """root_files: dict name -> bytes, or name -> dict (a folder).
    Returns (image bytes, hooks) laid out: volume descriptor at 0x10000,
    then the tables and file data. tweak(img_info) may damage it."""
    img = bytearray()
    cursor = [(0x10000 // SECTOR) + 1]  # next free sector, after the descriptor
    placed = []  # (sector, bytes)
    tables = {}

    def place(data):
        sec = cursor[0]
        placed.append((sec, data))
        cursor[0] += max(1, (len(data) + SECTOR - 1) // SECTOR)
        return sec

    def emit(folder, label):
        entries = []
        for name, val in folder.items():
            if isinstance(val, dict):
                sec, size = emit(val, label + "/" + name)
                entries.append((name, sec, size, 0x10))
            else:
                sec = place(val) if val else 0
                entries.append((name, sec, len(val), 0x20))
        t = table_for(entries) if entries else b""
        t = pad(t)
        sec = place(t)
        tables[label] = (sec, len(t))
        return sec, len(t)

    rsec, rsize = emit(root_files, "")
    info = {"root": (rsec, rsize), "tables": tables, "placed": placed, "partition": partition}
    if tweak:
        tweak(info)
    rsec, rsize = info["root"]
    vol = bytearray(SECTOR)
    vol[0:20] = MAGIC
    struct.pack_into("<II", vol, 20, rsec, rsize)
    vol[0x7EC:0x7EC + 20] = MAGIC
    end = max(s * SECTOR + len(d) for s, d in info["placed"])
    total = partition + max(end, 0x10000 + SECTOR)
    out = bytearray(total)
    out[partition + 0x10000:partition + 0x10000 + SECTOR] = vol
    for s, d in info["placed"]:
        a = partition + s * SECTOR
        out[a:a + len(d)] = d
    return bytes(out), info


def disc(maps, extra_root=None):
    root = {"default.xbe": b"XBEH" + bytes(5000), "xvmc": {"a.bin": b"decoy"}, "maps": maps}
    if extra_root:
        root.update(extra_root)
    return root


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    good = good_maps()
    ref = os.path.join(outdir, "ref", "maps")
    os.makedirs(ref, exist_ok=True)
    for n, d in good.items():
        open(os.path.join(ref, n), "wb").write(d)
    expect = []

    def write(name, data, verdict):
        open(os.path.join(outdir, name + ".xiso"), "wb").write(data)
        expect.append("%s %s" % (name, verdict))

    img, _ = build_image(disc(good))
    write("good", img, "ok")
    write("good_xgd", build_image(disc(good), partition=0x02080000)[0], "ok")
    write("trunc_middle", img[: len(img) // 2], "refuse")
    write("trunc_tiny", img[:0x10200], "refuse")
    write("empty", b"", "refuse")
    write("notxbox", bytes(random.Random(1).getrandbits(8) for _ in range(300000)), "refuse")

    # a 7.x (PC/Mac) maps folder, a bad signature, a missing map
    v7 = dict(good)
    v7["ui.map"] = cache_file("ui", 20000, version=7)
    write("version7", build_image(disc(v7))[0], "refuse")
    bad = dict(good)
    bad["a30.map"] = cache_file("a30", 20000, head=b"head")
    write("badsignature", build_image(disc(bad))[0], "refuse")
    bad = dict(good)
    bad["a30.map"] = cache_file("a30", 20000, foot=b"\0\0\0\0")
    write("badfooter", build_image(disc(bad))[0], "refuse")
    bad = dict(good)
    bad["a30.map"] = b"daeh" + bytes(3000)
    write("zeroheader", build_image(disc(bad))[0], "refuse")
    noa10 = {k: v for k, v in good.items() if k != "a10.map"}
    write("no_a10", build_image(disc(noa10))[0], "refuse")
    noui = {k: v for k, v in good.items() if k != "ui.map"}
    write("no_ui", build_image(disc(noui))[0], "refuse")
    write("no_maps_folder", build_image({"default.xbe": b"x" * 100})[0], "refuse")

    # names that leave the folder or are not plain: skipped, the rest kept
    evil = dict(good)
    evil["../escape.map"] = cache_file("escape", 5000)
    evil["sub/b.map"] = cache_file("sub", 5000)
    evil[".."] = cache_file("dots", 5000)
    evil["a\\b.map"] = cache_file("bs", 5000)
    evil["bad:name.map"] = cache_file("colon", 5000)
    evil["nul\x01.map"] = cache_file("ctl", 5000)
    write("evilnames", build_image(disc(evil))[0], "ok")
    # ... and when ui.map exists only under an evil name
    evil2 = {k: v for k, v in good.items() if k != "ui.map"}
    evil2["../ui.map"] = cache_file("ui", 5000)
    write("evil_ui_only", build_image(disc(evil2))[0], "refuse")

    # damaged file systems (a cycle of links among the maps entries)
    def cycle(info):
        sec, size = info["tables"]["/maps"]
        new = []
        for s, d in info["placed"]:
            if s == sec:
                d = bytearray(d)
                # find the second entry (unit of the first entry's right link,
                # or any other entry) and point its left link at unit 0 would be "none";
                # point right links of every entry at the next entry and the last
                # back at the second, which cycles without unit 0
                pos = 0
                units = []
                while pos + 14 <= len(d) and not (d[pos] == 0xFF and d[pos + 1] == 0xFF):
                    n = d[pos + 13]
                    units.append(pos // 4)
                    pos += (14 + n + 3) & ~3
                for a in range(len(units)):
                    struct.pack_into("<HH", d, units[a] * 4, 0, units[(a + 1) % len(units)] if units[(a + 1) % len(units)] else units[1])
                new.append((s, bytes(d)))
            else:
                new.append((s, d))
        info["placed"][:] = new
    write("dirloop", build_image(disc(good), tweak=cycle)[0], "either")

    def self_left(info):
        # every entry's left AND right links point at the entry itself
        # (the first is at unit 0, so use the second entry as the table's
        # reachable loop through the first's right link)
        sec, size = info["tables"]["/maps"]
        new = []
        for s, d in info["placed"]:
            if s == sec:
                d = bytearray(d)
                second = ((14 + d[13] + 3) & ~3) // 4
                struct.pack_into("<HH", d, 0, second, second)
                struct.pack_into("<HH", d, second * 4, second, second)
                new.append((s, bytes(d)))
            else:
                new.append((s, d))
        info["placed"][:] = new
    write("selfloop", build_image(disc(good), tweak=self_left)[0], "either")

    def sector_out(info):
        sec, size = info["tables"]["/maps"]
        new = []
        for s, d in info["placed"]:
            if s == sec:
                d = bytearray(d)
                pos = 0
                while pos + 14 <= len(d) and not (d[pos] == 0xFF and d[pos + 1] == 0xFF):
                    n = d[pos + 13]
                    if bytes(d[pos + 14:pos + 14 + n]) == b"a10.map":
                        struct.pack_into("<I", d, pos + 4, 0xFFFFFFF0)
                    pos += (14 + n + 3) & ~3
                new.append((s, bytes(d)))
            else:
                new.append((s, d))
        info["placed"][:] = new
    write("sector_out_of_range", build_image(disc(good), tweak=sector_out)[0], "refuse")

    def huge_size(info):
        sec, size = info["tables"]["/maps"]
        new = []
        for s, d in info["placed"]:
            if s == sec:
                d = bytearray(d)
                pos = 0
                while pos + 14 <= len(d) and not (d[pos] == 0xFF and d[pos + 1] == 0xFF):
                    n = d[pos + 13]
                    if bytes(d[pos + 14:pos + 14 + n]) == b"a10.map":
                        struct.pack_into("<I", d, pos + 8, 0xFFFFFFFF)
                    pos += (14 + n + 3) & ~3
                new.append((s, bytes(d)))
            else:
                new.append((s, d))
        info["placed"][:] = new
    write("file_size_4gb", build_image(disc(good), tweak=huge_size)[0], "refuse")

    def huge_root(info):
        info["root"] = (info["root"][0], 0xFFFFFFFF)
    write("root_size_huge", build_image(disc(good), tweak=huge_root)[0], "refuse")

    def root_sector_out(info):
        info["root"] = (0xFFFFFF00, info["root"][1])
    write("root_sector_out", build_image(disc(good), tweak=root_sector_out)[0], "refuse")

    def name_overrun(info):
        sec, size = info["tables"]["/maps"]
        new = []
        for s, d in info["placed"]:
            if s == sec:
                d = bytearray(d)
                pos = 0
                last = 0
                while pos + 14 <= len(d) and not (d[pos] == 0xFF and d[pos + 1] == 0xFF):
                    n = d[pos + 13]
                    last = pos
                    pos += (14 + n + 3) & ~3
                d[last + 13] = 255  # name length runs past the table's end
                new.append((s, bytes(d)[: last + 20]))
            else:
                new.append((s, d))
        info["placed"][:] = new
    write("name_overrun", build_image(disc(good), tweak=name_overrun)[0], "either")

    # a good image with a big file: progress and cancel testing (40 MB)
    big = dict(good)
    big["bloodgulch.map"] = cache_file("bloodgulch", 40 * 1024 * 1024)
    write("big", build_image(disc(big))[0], "ok")

    open(os.path.join(outdir, "expect.txt"), "w").write("\n".join(expect) + "\n")


if __name__ == "__main__":
    main(sys.argv[1])
