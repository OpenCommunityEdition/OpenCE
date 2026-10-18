#!/usr/bin/env python3
"""Check native renderer record copies against their decomp source witnesses."""
from __future__ import print_function
import os
import re

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def read(relative):
    with open(os.path.join(root, relative), "r") as source:
        return source.read()

renderer = read("port/mac/mac_object_renderer.c")
models = read("source/models/models.c")
model_types = read("source/models/model_definitions.h")
rasterizer = read("source/rasterizer/rasterizer.c")
geometry = read("source/rasterizer/rasterizer_model_types.h")

def fields(source, name):
    match = re.search(r"struct\s+" + re.escape(name) + r"\s*\{(.*?)\};", source, re.S)
    if not match:
        raise SystemExit("missing source witness: " + name)
    no_comments = re.sub(re.compile(r"/\*.*?\*/", re.S), "", match.group(1))
    return re.sub(r"\s+", " ", no_comments).strip()

def check(copy_name, source_text, source_name):
    actual = fields(renderer, copy_name)
    expected = fields(source_text, source_name)
    if actual != expected:
        raise SystemExit("{} diverges from source {}:\n{}\n{}".format(copy_name, source_name, actual, expected))

check("mac_model_shader_reference", models, "model_shader_reference")
check("mac_model_geometry", models, "model_geometry")
check("mac_model_geometry_part", models, "model_geometry_part")
if "byte nodes[2];" not in fields(geometry, "model_vertex_compressed"):
    raise SystemExit("public model vertex node fields changed")
if "((real)value*2.0f+1.0f)*(1.0f/65535.0f)" not in re.sub(r"\s+", "", renderer):
    raise SystemExit("compressed UV decode no longer follows source normalization")
if "matrix4x3_transform_point" not in rasterizer or "node_weight0" not in rasterizer:
    raise SystemExit("engine skinning source witness missing")
if "real_matrix4x3 runtime_default_inverse_matrix;" not in fields(model_types, "model_node"):
    raise SystemExit("model node inverse-bind matrix source witness missing")
if not re.search(r"matrix4x3_multiply\(\s*&node_matrices\[node_index\],\s*&node->runtime_default_inverse_matrix,\s*&relative_node_matrices\[node_index\]\s*\)", models):
    raise SystemExit("models.c live-node * inverse-bind transform source witness changed")
if "runtime_default_inverse_matrix" not in renderer or "skin_matrices[node_index]" not in renderer:
    raise SystemExit("diagnostic renderer does not apply the model inverse-bind palette")
if "NUMBER_OF_DETAIL_LEVELS_PER_MODEL = 5" not in models or "geometry_detail_level_index = NUMBER_OF_DETAIL_LEVELS_PER_MODEL-1" not in models:
    raise SystemExit("highest detail slot source witness missing")
print("native object renderer source layouts: 4 witnesses, UV/skinning decode and LOD selection pass")
