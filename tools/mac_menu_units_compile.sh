#!/bin/sh
# Compile menu units with the rendered session's engine flags (diagnostic).
# usage: BASE=dir OUT=dir ARCHS="i386 ppc" mac_menu_units_compile.sh unit.c...
set -u
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
base=${BASE:?set BASE to the directory of the rendered session headers}
out=$base
audit=${MAC_RENDER_AUDIT_BASE:-build/mac-audit}
dest=${OUT:?}
mkdir -p "$dest"
for arch in ${ARCHS:-i386 ppc}; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    for source in "$@"; do
        name=$(basename "$source" .c)
        "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp ${EXTRA:-} -fms-extensions -fshort-wchar \
        -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections \
        -I"$out" -I"$out/render-graph" -I"$audit" -I. \
        -Isource -Isource/cseries -Isource/math -Isource/physics \
        -Isource/tag_files -Isource/memory -Isource/cache -Isource/camera \
        -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking \
        -Isource/ai -Isource/interface -Isource/rasterizer \
        -Isource/rasterizer/xbox -Isource/text -Isource/cutscene \
        -Isource/shaders -Isource/game -Isource/input -Isource/objects \
        -Isource/items -Isource/units -Isource/models -Isource/scenario \
        -Isource/structures -Isource/render -Isource/sound -Isource/effects \
        -Isource/shell -Isource/bink -I"source/saved games" \
        -Iport/mac -Iport/linux/game -Iport/linux/src -Iport/third_party/expat \
        -Iport/third_party/stb_vorbis -idirafter port/linux/include \
        -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h \
        -include "$audit/mac_spawn_witness_names.h" \
        -include port/include/halo_math.h \
        -include port/linux/include/halo_linux_source_fixups.h \
        -include "$audit/mac_engine_semantics.h" \
        -c "$source" -o "$dest/${name}_$arch.o" > "$dest/${name}_$arch.o.txt" 2>&1 \
        && echo "ok $arch $name" || { echo "FAIL $arch $name"; grep -E "error" "$dest/${name}_$arch.o.txt" | head -${ERRS:-15}; }
    done
done
