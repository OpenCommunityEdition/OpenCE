#!/usr/bin/env python3
"""Keep bounded model and Chicago base-map readers tied to source layouts."""
from __future__ import print_function
import os
import re

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def read(path):
    with open(os.path.join(root, path), "r") as source:
        return source.read()

source = read("source/rasterizer/xbox/rasterizer_xbox_models.c")
chicago_source = read("source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c")
reader = read("port/mac/retail_texture.c")
properties = re.search(r"struct\s+shader_model_properties\s*\{(.*?)\};", source, re.S)
if not properties:
    raise SystemExit("missing decomp shader_model_properties layout")
fields = properties.group(1)
for declaration in ("real map_u_scale;", "real map_v_scale;", "struct tag_reference base_map;"):
    if declaration not in fields:
        raise SystemExit("shader model field witness missing: " + declaration)
if "offsetof(struct shader_model_definition, model.map_u_scale) == 0x9C" not in re.sub(r"\s+", " ", source):
    raise SystemExit("shader model map_u_scale offset witness missing")
if "offsetof(struct shader_model_definition, model.base_map.index) == 0xB0" not in re.sub(r"\s+", " ", source):
    raise SystemExit("shader model base_map offset witness missing")
if not re.search(r"real\s+map_u_scale\s*;\s*real\s+map_v_scale\s*;", fields):
    raise SystemExit("map_v_scale is no longer adjacent to map_u_scale")
for offset in ("0x9C", "0xA0", "0xB0"):
    if offset not in reader:
        raise SystemExit("native soso reader no longer uses witnessed offset " + offset)
if "0x736f736f" not in reader or "0x6269746d" not in reader:
    raise SystemExit("soso/bitm group validation missing")
map_struct = re.search(r"struct\s+shader_transparent_chicago_map\s*\{(.*?)\};", chicago_source, re.S)
if not map_struct:
    raise SystemExit("missing source Chicago map record")
map_fields = map_struct.group(1)
if "sizeof(struct shader_transparent_chicago_map) == 0xDC" not in re.sub(r"\s+", " ", chicago_source):
    raise SystemExit("missing Chicago map size witness")
if "offsetof(struct shader_transparent_chicago_map, map_u_scale) == 0x54" not in re.sub(r"\s+", " ", chicago_source):
    raise SystemExit("missing Chicago U-scale witness")
if not all(field in map_fields for field in ("real map_v_scale;", "real map_u_offset;",
        "real map_v_offset;", "real map_rotation;", "struct tag_reference map;")):
    raise SystemExit("Chicago base-map transform/reference fields changed")
for offset in ("0x54", "0x58", "0x5C", "0x60", "0x64", "0x6C", "0x78", "0xDC"):
    if offset not in reader:
        raise SystemExit("native Chicago reader no longer uses witnessed offset " + offset)
if "mac_retail_chicago_base_bitmap" not in reader:
    raise SystemExit("bounded Chicago first-map reader missing")
print("soso offsets match model source; Chicago first-map fields match 0xDC source record and 0x54..0x78 offsets")
