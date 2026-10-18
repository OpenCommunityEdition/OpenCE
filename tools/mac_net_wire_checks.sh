#!/bin/sh
# The network wire's byte order (port/linux/include/halo_endian.h): the game's
# units that put data on the wire compile for ppc and i386, and the golden
# vectors (tools/net_wire_golden.c, whose expected bytes were made on a
# little-endian x86 host) come out the same where they run.
# Needs mac_engine_semantics.h in $MAC_CHECK_DIR (as the other mac_*_checks).
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mac_archs=${MAC_NET_ARCHS:-ppc i386}
mac_run_archs=${MAC_NET_RUN_ARCHS:-i386 ppc}
mkdir -p "$mac_dir/net-wire"
default_units='source/bungie_net/common/message_header.c source/memory/data_encoding.c source/bungie_net/network/transport_endpoint_winsock.c source/bungie_net/network/transport_endpoint_set_winsock.c source/bungie_net/common/public_key_crypt.c source/game/game_engine.c source/game/game_engine_slayer.c source/game/game_engine_ctf.c source/game/game_engine_oddball.c source/game/game_engine_king.c source/game/game_engine_race.c port/linux/game/network_distributed.c port/linux/game/network_objects.c port/linux/game/network_damage.c port/linux/game/network_actors.c port/linux/game/network_coop.c port/linux/game/network_votekick.c port/linux/game/network_voice.c port/linux/game/map_share.c'
units=${MAC_NET_UNITS:-$default_units}
cat > "$mac_dir/net-wire/names.h" <<'NAMES'
#define MAC_SPAWN_JOIN_INNER(a,b) a##b
#define MAC_SPAWN_JOIN(a,b) MAC_SPAWN_JOIN_INNER(a,b)
#define verify_render_model_effect_size MAC_SPAWN_JOIN(mac_effect_witness_,__LINE__)
#define verify_rasterizer_model_begin_parameters_size MAC_SPAWN_JOIN(mac_begin_witness_,__LINE__)
#define effect_definition_size_assert MAC_SPAWN_JOIN(mac_effect_definition_witness_,__LINE__)
#define effect_event_definition_size_assert MAC_SPAWN_JOIN(mac_effect_event_witness_,__LINE__)
#define verify_model_vertex_uncompressed_size MAC_SPAWN_JOIN(mac_vertex_uncompressed_witness_,__LINE__)
#define verify_model_vertex_compressed_size MAC_SPAWN_JOIN(mac_vertex_compressed_witness_,__LINE__)
NAMES
status=0
for arch in $mac_archs; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    for unit in $units; do
        name=$(basename "$unit" .c)
        if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/memory \
            -Isource/cache -Isource/camera -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking -Isource/ai -Isource/interface -Isource/rasterizer -Isource/text -Isource/cutscene -Isource/shaders -Isource/game -Isource/input -Isource/objects -Isource/items -Isource/units -Isource/models \
            -Isource/scenario -Isource/structures -Isource/render -Isource/sound -Isource/effects -Isource/shell \
            -I"source/saved games" -Iport/mac -Iport/linux/game -idirafter port/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
            -include port/mac/engine_xdk_prefix.h \
            -include "$mac_dir/net-wire/names.h" -include port/include/halo_math.h \
            -include port/linux/include/halo_linux_source_fixups.h \
            -include "$mac_dir/mac_engine_semantics.h" -c "$unit" -o "$mac_dir/net-wire/${name}_$arch.o" \
            > "$mac_dir/net-wire/${name}_$arch.txt" 2>&1; then
            echo "compiled $arch $unit"
        else
            echo "FAILED   $arch $unit (see $mac_dir/net-wire/${name}_$arch.txt)"
            status=1
        fi
    done
done
# the golden bytes: build/net-wire/extracted.h is made from the sources by
# python3 tools/net_wire_extract.py build/net-wire/extracted.h (on a host that has python 3)
if [ -f build/net-wire/extracted.h ]; then
    for arch in $mac_archs; do
        if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 -Wall -fms-extensions \
            -Ibuild/net-wire -Itools -idirafter port/linux/include tools/net_wire_golden.c \
            -o "$mac_dir/net-wire/golden_$arch" > "$mac_dir/net-wire/golden_$arch.txt" 2>&1; then
            echo "built golden $arch"
        else
            echo "FAILED   golden $arch (see $mac_dir/net-wire/golden_$arch.txt)"
            status=1
        fi
    done
    for arch in $mac_run_archs; do
        # (a ppc program runs under Rosetta, where it is installed)
        runner=; if [ "$arch" = ppc ]; then runner="arch -ppc"; fi
        if [ -x "$mac_dir/net-wire/golden_$arch" ]; then
            echo "running golden $arch"
            $runner "$mac_dir/net-wire/golden_$arch" --layouts || status=1
        fi
    done
else
    echo "no build/net-wire/extracted.h: the golden test was not built"
    status=1
fi
exit $status
