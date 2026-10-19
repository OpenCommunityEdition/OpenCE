#!/bin/sh
# Check the shader texts a run dumped (HALO_MAC_GLSL_DUMP=<folder>) with glslangValidator, as the GLSL
# version given (default 110, Mac OS X 10.4's; 120 is 10.5's). The first line of each text is replaced by
# that version, so a folder dumped by a 120 run checks the 110 text too, but only a run with
# HALO_MAC_GLSL_VERSION=110 (or a 1.10 driver) dumps what the port itself makes for 1.10.
# usage: sh tools/mac_glsl_validate.sh <folder> [110|120]
set -u
dir=${1:?usage: mac_glsl_validate.sh folder [110|120]}
version=${2:-110}
validator=${GLSLANG:-glslangValidator}
total=0
bad=0
tmp=$(mktemp -d "${TMPDIR:-/tmp}/glslcheck.XXXXXX")
for file in "$dir"/*.vert "$dir"/*.frag; do
    [ -f "$file" ] || continue
    case "$file" in *.vert) stage=vert ;; *) stage=frag ;; esac
    total=$((total + 1))
    copy="$tmp/$(basename "$file")"
    sed "1s/^#version [0-9]*/#version $version/" "$file" > "$copy"
    if ! out=$("$validator" -S "$stage" "$copy" 2>&1); then
        bad=$((bad + 1))
        echo "FAIL $file"
        echo "$out" | sed -n '1,6p'
    fi
done
rm -rf "$tmp"
echo "GLSL $version: $total shaders, $bad with errors"
[ "$bad" -eq 0 ] && [ "$total" -gt 0 ]
