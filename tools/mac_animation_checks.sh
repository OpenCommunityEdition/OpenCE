#!/bin/sh
# Whole engine units + unchanged vendored musl, never extracted function bodies.
# Retained compressed paths still import system floorf/lrintf/sqrtf. They are
# not exercised by the uncompressed base sampler; this does not establish
# complete deterministic engine math or a controlled FP rounding environment.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
# GCC 4 rejects the same layout witness typedef repeated in models.c and its
# shared rasterizer header. Keep BOTH expressions, give only their names a
# unique source-line suffix; this does not rewrite any engine implementation.
cat > "$mac_dir/mac_animation_witness_names.h" <<'NAMES'
#define MAC_ANIMATION_JOIN_INNER(a,b) a##b
#define MAC_ANIMATION_JOIN(a,b) MAC_ANIMATION_JOIN_INNER(a,b)
#define verify_render_model_effect_size MAC_ANIMATION_JOIN(mac_effect_witness_,__LINE__)
#define verify_rasterizer_model_begin_parameters_size MAC_ANIMATION_JOIN(mac_begin_witness_,__LINE__)
NAMES
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    objects=
    for unit in source/models/model_animations.c source/models/model_animation_definitions.c source/models/models.c source/math/real_math.c port/mac/engine_services.c port/mac/player_services.c port/mac/engine_animation.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
            -Isource/render -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
            -include port/mac/engine_prefix.h -include "$mac_dir/mac_animation_witness_names.h" -include port/include/halo_math.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_animation_${arch}.o"
        objects="$objects $mac_dir/${name}_animation_${arch}.o"
    done
    for unit in port/third_party/musl-math/src/*.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu99 -O2 $mac_fp -Iport/third_party/musl-math/include \
            -include port/third_party/musl-math/include/libm.h \
            -c "$unit" -o "$mac_dir/${name}_musl_animation_${arch}.o"
        objects="$objects $mac_dir/${name}_musl_animation_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fno-strict-aliasing -Iport/mac -Wl,-dead_strip \
        $objects port/mac/engine_format.c port/mac/retail_cache.c port/mac/retail_animation.c port/mac/retail_skeleton.c \
        tools/mac_engine_animation_probe.c -o "$mac_dir/engine_animation_$arch"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fshort-wchar -fno-strict-aliasing \
        -I. -Isource -Isource/cseries -Isource/math -Isource/models -Isource/tag_files \
        -Iport/mac -Iport/include/xdk -idirafter port/linux/include -Wl,-dead_strip \
        $objects port/mac/engine_format.c port/mac/retail_cache.c port/mac/retail_animation.c port/mac/retail_skeleton.c \
        tools/mac_aiming_guard_probe.c -o "$mac_dir/aiming_guard_$arch"
    nm -u "$mac_dir/engine_animation_$arch" > "$mac_dir/animation_imports_$arch.txt"
    if nm -m -u "$mac_dir/engine_animation_$arch" | grep 'weak'; then
        echo "Unresolved weak engine import" >&2; exit 1
    fi
    if awk '{print $NF}' "$mac_dir/animation_imports_$arch.txt" | grep -E '^_(sin|cos|tan|asin|acos|atan|atan2|exp|log|log2|log10|pow)$'; then
        echo "Unsafe system transcendental math import" >&2; exit 1
    fi
done
"$mac_dir/engine_animation_i386" "$@"
"$mac_dir/aiming_guard_i386"
