#!/usr/bin/env python3
"""The engine's own texture cache (source/cache/xbox_texture_cache.c) for the
Mac build, which already has a texture cache of its own for the native v7
session (port/mac/engine_texture_lifecycle.c and engine_texture_cache.c).
Its entry points are named xbox_... here, and the Mac's dispatch to them when
the run plays Xbox maps (mac_xbox_mode)."""
import sys

RENAMES = (
    ("texture_cache_new", "xbox_texture_cache_new"),
    ("texture_cache_delete", "xbox_texture_cache_delete"),
    ("texture_cache_open", "xbox_texture_cache_open"),
    ("texture_cache_close", "xbox_texture_cache_close"),
    ("texture_cache_idle", "xbox_texture_cache_idle"),
    ("texture_cache_flush", "xbox_texture_cache_flush"),
    ("texture_cache_bitmap_new", "xbox_texture_cache_bitmap_new"),
    ("texture_cache_bitmap_delete", "xbox_texture_cache_bitmap_delete"),
    ("texture_cache_steal_memory", "xbox_texture_cache_steal_memory"),
    ("texture_cache_return_memory", "xbox_texture_cache_return_memory"),
    ("texture_cache_debug_render", "xbox_texture_cache_debug_render"),
    ("_texture_cache_bitmap_get_hardware_format", "xbox_texture_cache_bitmap_get_hardware_format"),
    ("texture_cache_debug_options", "xbox_texture_cache_debug_options"),
    ("debug_texture_cache", "xbox_debug_texture_cache"),
)


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: mac_xbox_texture_cache_view.py xbox_texture_cache.c output.c")
    text = open(sys.argv[1], "r").read()
    out = open(sys.argv[2], "w")
    for old, new in RENAMES:
        out.write("#define %s %s\n" % (old, new))
    out.write(text)
    out.close()


if __name__ == "__main__":
    main()
