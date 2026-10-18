#!/bin/sh
# Builds tools/mac_vertex_path_probe.c for PowerPC (run on the Intel build Mac, 10.6 host).
# usage: mac_vertex_path_checks.sh [outdir]   (default: current directory)
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${1:-.}
here=$(dirname "$0")
"$mac_cc" -arch ppc -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -mno-fused-madd -O2 \
    -Wall -Wno-unused -DGL_GLEXT_PROTOTYPES \
    "$here/mac_vertex_path_probe.c" -o "$out/vertex_path_probe" -framework OpenGL
