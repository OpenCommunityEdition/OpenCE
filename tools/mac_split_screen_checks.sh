#!/bin/sh
# Split screen started without the menus (HALO_MAC_LOCAL_PLAYERS, debug.mac_local_players): two local players on a
# multiplayer map (Blood Gulch) and on a campaign level (b30, co-op), each on a virtual pad, so that a change that
# breaks local players, the second view or its HUD fails here and not at a person's second pad.
# The session's own checks fail unless every player is in the game (tools/mac_engine_session_probe.c, "split screen:").
# usage: sh tools/mac_split_screen_checks.sh SESSION_BINARY DATA_ROOT OUTDIR
#   DATA_ROOT holds maps/ (the Xbox maps: bloodgulch.map and b30.map) or is GameData with Maps/ (the retail form,
#   which also needs bitmaps.map and sounds.map there). Prints "checks, N failures" like the other checks.
# HALO_MAC_MENU_ART_DIR must name port/assets/menus outside a kit.
set -u
binary=$1; root=$2; out=$3
mkdir -p "$out"
if [ -d "$root/maps" ]; then maps=$root/maps; extra=; else maps=$root/Maps; extra="$maps/bitmaps.map $maps/sounds.map"; fi
checks=0; failures=0
check() { checks=$((checks+1)); if [ "$1" != 1 ]; then failures=$((failures+1)); echo "FAIL: $2"; fi; }
run() { # name map frames campaign players
    name=$1; map=$2; frames=$3; campaign=$4; players=$5
    d=$out/$name; rm -rf "$d"; mkdir -p "$d/save" "$d/cache"
    ( cd "$d" && env HALO_MAC_LOCAL_PLAYERS=$players HALO_MAC_CAMPAIGN=$campaign HALO_MAC_LOCAL_PLAYER_LOG=60 \
        HALO_MAC_RENDER_FIXED_STEP=1 HALO_MAC_RENDER_SEED=1835103336 HALO_MAC_RENDER_FRAMES=$frames \
        HALO_MAC_DATA_ROOT="$root" HALO_MAC_MAP_CACHE="$d/cache" HALO_MAC_SAVE_ROOT="$d/save" \
        HALO_MAC_CONFIG_DIR="$d/cfg" HALO_MAC_RENDER_CAPTURE="$d/frame.ppm" \
        "$binary" "$maps/$map.map" $extra --gpu-render > "$d/out.txt" 2>&1 )
    status=$?
    check $([ $status = 0 ] && echo 1 || echo 0) "$name: the run exits 0 (exit $status)"
    check $(grep -a "session lifecycle" "$d/out.txt" | tail -1 | grep -q " 0 failures" && echo 1 || echo 0) \
        "$name: the session's own checks have no failures"
    if [ "$players" -gt 1 ]; then
        check $(grep -a -q "split screen: $players local players wanted, $players present" "$d/out.txt" && echo 1 || echo 0) \
            "$name: $players local players are in the game"
        # the second player has a pad (the log line of HALO_MAC_LOCAL_PLAYER_LOG)
        check $(grep -a "local players: frame" "$d/out.txt" | grep -q "controller 1 .* pad yes" && echo 1 || echo 0) \
            "$name: controller 2 has a player and a pad"
    fi
    echo "$name: $(grep -a 'session lifecycle' "$d/out.txt" | tail -1 | sed 's/.*lifecycle: //' | cut -c1-80)"
}
run bloodgulch2 bloodgulch ${FRAMES_MP:-300} 0 2
run b30coop b30 ${FRAMES_COOP:-1200} 1 2
run bloodgulch1 bloodgulch ${FRAMES_MP:-300} 0 1
a=$(md5 -q "$out/bloodgulch2/frame.ppm" 2>/dev/null || md5sum "$out/bloodgulch2/frame.ppm" | cut -d' ' -f1)
b=$(md5 -q "$out/bloodgulch1/frame.ppm" 2>/dev/null || md5sum "$out/bloodgulch1/frame.ppm" | cut -d' ' -f1)
check $([ "$a" != "$b" ] && echo 1 || echo 0) "two players draw a different frame from one"
echo "split screen: $checks checks, $failures failures"
[ $failures = 0 ]
