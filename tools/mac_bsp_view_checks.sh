#!/bin/sh
# Build/run a hidden first-person BSP diagnostic on the MacBook/G4 test hosts.
set -eu
if [ "$#" -lt 2 ]; then
    echo "usage: $0 map bitmaps.map [spawn-index [eye-height [yaw-offset [pitch]]]]" >&2
    exit 2
fi
map=$1
bitmaps=$2
shift 2
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_BSP_VIEW_DIR:-build/mac-bsp-view}
mkdir -p "$mac_dir"
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror $fp -Iport/mac \
        port/mac/retail_cache.c port/mac/retail_geometry.c port/mac/retail_texture.c \
        port/mac/retail_spawns.c port/mac/mac_bsp_renderer.c \
        port/mac/mac_input.c port/mac/mac_window.c tools/mac_bsp_view.c \
        -framework Carbon -framework AGL -framework OpenGL \
        -o "$mac_dir/mac_bsp_view_$arch"
done
run_arch=${MAC_RUN:-i386}
if [ "$run_arch" != none ];then
    output=${MAC_BSP_VIEW_OUTPUT:-$mac_dir/view-$run_arch.ppm}
    "$mac_dir/mac_bsp_view_$run_arch" "$map" "$bitmaps" "$output" "$@"
fi
