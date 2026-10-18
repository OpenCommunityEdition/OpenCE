#!/bin/sh
# Compile the netcode with the session's own flags and say what linking it
# into the session would still need: the game's networking
# (source/networking), the port's distributed netcode
# (port/linux/game/network_*.c, map_share*.c, coop_*.c) and the platform's
# sockets and XNet (port/mac/engine_sockets.c and friends).
#
# MAC_NET_LINK_SESSION is the session build's output directory, which must
# hold engine_objects_<arch>.txt (tools/mac_rendered_session_checks.sh with
# MAC_RENDERED_SESSION_BUILD_ONLY=1 writes it).
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
session=${MAC_NET_LINK_SESSION:-build/mac-rendered-session}
audit=${MAC_RENDER_AUDIT_BASE:-build/mac-audit}
out=${MAC_NET_LINK_OUT:-build/mac-net-link}
archs=${MAC_NET_LINK_ARCHS:-ppc i386}
mkdir -p "$out"

# The game's own networking, the port's netcode, and the platform's sockets.
game_units='source/networking/network_game_globals.c source/networking/network_messages.c source/networking/network_connection.c source/networking/network_game_manager.c source/networking/network_client_manager.c source/networking/network_server_manager.c source/networking/network_client_message_handler.c source/networking/network_server_message_handler.c source/networking/network_game_ui.c source/networking/network_game_preferences.c source/bungie_net/common/message_header.c source/bungie_net/common/public_key_crypt.c source/bungie_net/network/transport_endpoint_winsock.c source/bungie_net/network/transport_endpoint_set_winsock.c'
port_units='port/linux/game/network_distributed.c port/linux/game/network_objects.c port/linux/game/network_damage.c port/linux/game/network_actors.c port/linux/game/network_coop.c port/linux/game/network_votekick.c port/linux/game/network_voice.c port/linux/game/network_test.c port/linux/game/map_share.c port/linux/game/map_share_files.c port/linux/game/coop_enemies.c port/linux/game/coop_scripts.c port/linux/game/coop_spectate.c'
# XNet as the game declares it: these take the game's prefix, as the
# session's other engine units do.
prefix_units='port/mac/engine_winsock_init.c port/mac/engine_xnet_compat.c'
# These see the Darwin headers instead, and must not take the prefix.
platform_units='port/mac/engine_sockets.c port/mac/engine_xnet_status.c port/mac/mac_xnet_address.c port/linux/src/net_settings.c'
units=${MAC_NET_LINK_UNITS:-"$game_units $port_units $prefix_units"}

compile_engine_unit() {
    arch=$1; source=$2; object=$3
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    # shellcheck disable=SC2086
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -DHALO_MAC -fms-extensions -fshort-wchar \
        -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections \
        -I"$session" -I"$session/render-graph" -I"$audit" -I. \
        -Isource -Isource/cseries -Isource/math -Isource/physics \
        -Isource/tag_files -Isource/memory -Isource/cache -Isource/camera \
        -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking \
        -Isource/ai -Isource/interface -Isource/rasterizer \
        -Isource/rasterizer/xbox -Isource/text -Isource/cutscene \
        -Isource/shaders -Isource/game -Isource/input -Isource/objects \
        -Isource/items -Isource/units -Isource/models -Isource/scenario \
        -Isource/structures -Isource/render -Isource/sound -Isource/effects \
        -Isource/shell -Isource/bink -I"source/saved games" \
        -Iport/mac -Iport/linux/game -Iport/linux/src \
        -Iport/third_party/stb_vorbis -idirafter port/linux/include \
        -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h \
        -include "$audit/mac_spawn_witness_names.h" \
        -include port/include/halo_math.h \
        -include port/linux/include/halo_linux_source_fixups.h \
        -include "$audit/mac_engine_semantics.h" \
        -c "$source" -o "$object" > "$object.txt" 2>&1
}

compile_platform_unit() {
    arch=$1; source=$2; object=$3
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    # shellcheck disable=SC2086
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Wall -Wextra -D__int64='long long' -D__fastcall= \
        -Iport/mac -Iport/include/xdk -Iport/linux/include -Iport/linux/src \
        -c "$source" -o "$object" > "$object.txt" 2>&1
}

status=0
for arch in $archs; do
    objects=
    for unit in $units; do
        name=$(basename "$unit" .c)
        object="$out/${name}_$arch.o"
        if compile_engine_unit "$arch" "$unit" "$object"; then
            objects="$objects $object"
        else
            echo "FAILED   $arch $unit (see $object.txt)" >&2
            status=1
        fi
    done
    for unit in $platform_units; do
        name=$(basename "$unit" .c)
        object="$out/${name}_$arch.o"
        if compile_platform_unit "$arch" "$unit" "$object"; then
            objects="$objects $object"
        else
            echo "FAILED   $arch $unit (see $object.txt)" >&2
            status=1
        fi
    done
    echo "$arch: compiled $(printf '%s\n' $objects | grep -c .) network units"

    # What the netcode needs that the session does not already define. The
    # session's own objects are the ones its link uses; a name defined by one
    # of the units above is not a gap either.
    test -f "$session/engine_objects_$arch.txt" || {
        echo "no $session/engine_objects_$arch.txt: build the session first" >&2
        exit 2
    }
    # shellcheck disable=SC2046
    nm -g $(cat "$session/engine_objects_$arch.txt") 2>/dev/null |
        awk '$2 ~ /^[TDBSCRA]$/ {print $3}' | sort -u > "$out/defined_session_$arch.txt"
    # shellcheck disable=SC2086
    nm -g $objects 2>/dev/null |
        awk '$2 ~ /^[TDBSCRA]$/ {print $3}' | sort -u > "$out/defined_network_$arch.txt"
    # shellcheck disable=SC2086
    nm -u $objects 2>/dev/null | sed 's/^ *//' | grep '^_' | sort -u > "$out/undefined_network_$arch.txt"
    sort -u "$out/defined_session_$arch.txt" "$out/defined_network_$arch.txt" > "$out/defined_all_$arch.txt"
    comm -23 "$out/undefined_network_$arch.txt" "$out/defined_all_$arch.txt" > "$out/gap_$arch.txt"
    # A name the netcode defines and the session already defines would make
    # the link ambiguous.
    comm -12 "$out/defined_network_$arch.txt" "$out/defined_session_$arch.txt" > "$out/duplicate_$arch.txt"
    echo "$arch: $(grep -c . < "$out/gap_$arch.txt") names the session does not define, $(grep -c . < "$out/duplicate_$arch.txt") it defines twice"
    echo "--- $arch: still needed"
    cat "$out/gap_$arch.txt"
    echo "--- $arch: defined twice"
    cat "$out/duplicate_$arch.txt"
done
exit $status
