#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_OBJECT_RENDERER_DIR:-build/mac-object-renderer}
mkdir -p "$out"
python tools/mac_object_renderer_boundary.py
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra $fp -fms-extensions -fshort-wchar \
        -fno-strict-aliasing -fwrapv -I. -Isource -Isource/cseries -Isource/math \
        -Isource/rasterizer -Isource/tag_files -Iport/mac -idirafter port/linux/include \
        -Iport/include/xdk -include port/mac/engine_xdk_prefix.h \
        -include port/linux/include/halo_port_capacity.h -include port/include/halo_math.h \
        -c port/mac/mac_object_renderer.c -o "$out/object_renderer_$arch.o"
    for unit in port/mac/mac_bsp_renderer.c port/mac/mac_runtime_view.c port/mac/mac_object_render_probe.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 -Wall -Wextra $fp -fms-extensions -fshort-wchar \
            -fno-strict-aliasing -fwrapv -I. -Isource -Isource/cseries -Isource/math \
            -Isource/rasterizer -Isource/tag_files -Iport/mac -idirafter port/linux/include \
            -Iport/include/xdk -include port/mac/engine_xdk_prefix.h \
            -include port/linux/include/halo_port_capacity.h -include port/include/halo_math.h \
            -c "$unit" -o "$out/${name}_$arch.o"
    done
done
