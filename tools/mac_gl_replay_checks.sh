#!/bin/sh
# Builds the GL recorder (DYLD interposer) and the replay tool for PowerPC (run on the Intel build Mac, 10.6 host).
# usage: mac_gl_replay_checks.sh [outdir]   (default: current directory)
# Needs gl_simple.inc next to this script's sources or in outdir: python3 tools/mac_gl_replay_gen.py gl_simple.inc
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${1:-.}
here=$(dirname "$0")
flags="-arch ppc -isysroot $mac_sdk -mmacosx-version-min=10.4 -mno-fused-madd -O2 -Wall -Wno-unused -DGL_GLEXT_PROTOTYPES -I$out -I$here"
"$mac_cc" $flags -dynamiclib "$here/mac_gl_record_interpose.c" -o "$out/gl_record.dylib" -framework OpenGL
"$mac_cc" $flags "$here/mac_gl_replay.c" -o "$out/gl_replay" -framework OpenGL
