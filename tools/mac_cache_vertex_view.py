#!/usr/bin/env python
"""Build a native view of the retail cache vertex-resource registrations."""
from __future__ import print_function
import sys


def extract(source, start, end):
    if source.count(start) != 1 or source.count(end) != 1:
        raise ValueError("expected one source boundary: " + start)
    return source[source.index(start):source.index(end)]


def replace_count(source, old, new, count):
    if source.count(old) != count:
        raise ValueError("expected %d occurrences of %s" % (count, old))
    return source.replace(old, new)


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: mac_cache_vertex_view.py cache_files_windows.c output.c")
    source = open(sys.argv[1], "r").read()
    out = open(sys.argv[2], "w")
    out.write('''#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "cache/cache_files.h"
#include "rasterizer/rasterizer.h"
#include <xtl.h>
#include <stddef.h>

struct cache_file_tag_instance;
''')
    out.write(extract(source, "struct cache_file_tag_header\n{", "\n};\n\nstruct cache_file_structure_bsp_header"))
    out.write("};\n")
    out.write(extract(source, "struct cache_file_structure_bsp_header\n{", "\n};\n\nstruct cache_file_header"))
    out.write("};\n")
    out.write('''
typedef char mac_tag_header_size_is_36[
    sizeof(struct cache_file_tag_header) == 36 ? 1 : -1];
typedef char mac_tag_vertex_array_offset_is_20[
    offsetof(struct cache_file_tag_header, vertex_buffers) == 20 ? 1 : -1];
typedef char mac_tag_index_array_offset_is_28[
    offsetof(struct cache_file_tag_header, index_buffers) == 28 ? 1 : -1];
typedef char mac_bsp_header_size_is_24[
    sizeof(struct cache_file_structure_bsp_header) == 24 ? 1 : -1];
typedef char mac_bsp_lightmap_array_offset_is_16[
    offsetof(struct cache_file_structure_bsp_header, lightmap_vertex_buffers) == 16 ? 1 : -1];
typedef char mac_bsp_signature_offset_is_20[
    offsetof(struct cache_file_structure_bsp_header, signature) == 20 ? 1 : -1];

''')
    bodies = extract(source,
        "void tags_header_register_vertex_and_index_buffers(\n",
        "void cache_files_dispose(\n")
    bodies = replace_count(bodies,
        "vertex_buffer->Common = D3DCOMMON_TYPE_VERTEXBUFFER | 1;",
        "vertex_buffer->Common = D3DCOMMON_TYPE_VERTEXBUFFER | 1;\n\t\tmac_engine_cache_resource_validate((D3DResource *)vertex_buffer, FALSE);", 3)
    bodies = replace_count(bodies,
        "index_buffer->Common = D3DCOMMON_TYPE_INDEXBUFFER | 1;",
        "index_buffer->Common = D3DCOMMON_TYPE_INDEXBUFFER | 1;\n\t\tmac_engine_cache_resource_validate((D3DResource *)index_buffer, FALSE);", 1)
    bodies = replace_count(bodies,
        "IDirect3DVertexBuffer8_BlockUntilNotBusy(vertex_buffer);",
        "mac_engine_cache_resource_validate((D3DResource *)vertex_buffer, TRUE);\n\t\tIDirect3DVertexBuffer8_BlockUntilNotBusy(vertex_buffer);", 3)
    bodies = replace_count(bodies,
        "IDirect3DIndexBuffer8_BlockUntilNotBusy(index_buffer);",
        "mac_engine_cache_resource_validate((D3DResource *)index_buffer, FALSE);\n\t\tIDirect3DIndexBuffer8_BlockUntilNotBusy(index_buffer);", 1)
    out.write('#include "engine_cache_resources.h"\n')
    out.write(bodies)
    out.close()


if __name__ == "__main__":
    main()
