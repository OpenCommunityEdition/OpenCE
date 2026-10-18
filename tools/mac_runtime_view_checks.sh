#!/bin/sh
set -eu
if [ "$#" -lt 2 ]; then
    echo "usage: $0 bloodgulch.map bitmaps.map" >&2
    exit 2
fi
map=$1
bitmaps=$2
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_RUNTIME_VIEW_DIR:-build/mac-runtime-view}
mkdir -p "$mac_dir"
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror $fp -Iport/mac \
        port/mac/retail_cache.c port/mac/retail_geometry.c port/mac/retail_texture.c \
        port/mac/retail_spawns.c port/mac/mac_bsp_renderer.c port/mac/mac_input.c \
        port/mac/mac_window.c port/mac/mac_runtime_view.c tools/mac_runtime_view_probe.c \
        -framework Carbon -framework AGL -framework OpenGL -o "$mac_dir/runtime_view_$arch"
done
run_arch=${MAC_RUN:-i386}
if [ "$run_arch" != none ];then
    "$mac_dir/runtime_view_$run_arch" "$map" "$bitmaps"
fi
