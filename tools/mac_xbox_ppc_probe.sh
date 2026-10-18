#!/bin/sh
# The Xbox maps on a PowerPC Mac. Run from the probe folder on the iBook
# (any folder on the iBook) with the game binary as ./game_x and the Xbox
# disc image's maps in ./maps (ui.map, bloodgulch.map, a10.map ...):
#     sh xbox_ppc_probe.sh [map ...]            (default: every map in ./maps)
# Hidden runs only, one game at a time, nothing outside this folder is written.
#
# For each map:
#   1. a verified load (120 frames): HALO_MAC_XBOX_VERIFY=1 walks the map's tags and bsps before and after
#      the byte-swap and compares what the two walks found ("xbox swap verify ok" lines in out-verify.txt, or
#      the first field that differs), then the engine plays the converted tags
#   2. a render of 300 fixed steps (ui: its menu, 10 steps), the frame saved as frame-render.ppm
# The campaign maps (a10 a30 a50 b30 b40 c10 c20 c40 d20 d40) run as the campaign, the multiplayer maps
# without it, ui as the title menu (which needs the menu pictures: copy the kit's "menus" folder next to
# game_x and ./maps, or set HALO_MAC_MENU_ART_DIR; the ui run is skipped, and said so, without them). results.txt has one line per run (seconds, the session's check count,
# the frame's md5), then the lines that say why a run failed; send it and the runs/*/out-*.txt of any
# map that failed. HALO_MAC_XBOX_PPC=refuse in the environment switches the Xbox maps off.
cd "$(dirname "$0")" || exit 1
here=$(pwd)
[ -x ./game_x ] || { echo "./game_x is missing"; exit 1; }
[ -f ./maps/ui.map ] || { echo "./maps/ui.map is missing (copy the Xbox maps into ./maps)"; exit 1; }
if [ $# -gt 0 ]; then maps="$*"; else maps=`ls maps | sed -n 's/\.map$//p'`; fi
unset HALO_MAC_CAMPAIGN HALO_MAC_MENU
art=${HALO_MAC_MENU_ART_DIR:-}
[ -n "$art" ] || { [ -d ./menus ] && art="$here/menus"; }
[ -n "$art" ] || { [ -d ../menus ] && art=`cd .. && pwd`/menus; }
[ -n "$art" ] && export HALO_MAC_MENU_ART_DIR="$art"
rm -rf runs; mkdir -p runs cache
: > results.txt
echo "xbox maps on `uname -m`, `date`" >> results.txt
md5 -q ./game_x >> results.txt 2>/dev/null
for map in $maps; do
    d="$here/runs/$map"; mkdir -p "$d/save"
    case "$map" in
        a10|a30|a50|b30|b40|c10|c20|c40|d20|d40) kind="HALO_MAC_CAMPAIGN=1" ;;
        ui) kind="HALO_MAC_MENU=1" ;;
        *) kind="HALO_MAC_PROBE_KIND=multiplayer" ;;
    esac
    if [ "$map" = ui ] && [ -z "$art" ]; then
        echo "ui: skipped (no menus folder: put the kit's menus folder here or set HALO_MAC_MENU_ART_DIR)" >> results.txt
        continue
    fi
    for mode in verify render; do
        o="$d/out-$mode.txt"
        if [ "$mode" = verify ]; then frames=120; v=1; else frames=300; v=0; fi
        [ "$map" = ui ] && frames=10
        start=`date +%s`
        ( cd "$d" && env $kind HALO_MAC_XBOX_VERIFY=$v HALO_MAC_MAP_CACHE="$here/cache" HALO_MAC_DATA_ROOT="$here" HALO_MAC_SAVE_ROOT="$d/save" \
            HALO_MAC_RENDER_FIXED_STEP=1 HALO_MAC_RENDER_SEED=1835103336 HALO_MAC_RENDER_FRAMES=$frames \
            HALO_MAC_RENDER_CAPTURE="$d/frame-$mode.ppm" HALO_MAC_RENDER_TIMING=1 \
            nice "$here/game_x" "$here/maps/$map.map" --gpu-render > "$o" 2>&1; echo "exit $?" >> "$o" )
        end=`date +%s`
        printf '%s %s: %s seconds, %s, %s' "$map" "$mode" `expr $end - $start` "`grep 'session lifecycle' "$o" | tail -1 | sed 's/.*lifecycle: //'`" "`tail -1 "$o"`" >> results.txt
        if [ -f "$d/frame-$mode.ppm" ]; then printf ', frame %s' "`md5 -q "$d/frame-$mode.ppm"`" >> results.txt; fi
        printf '\n' >> results.txt
        grep "xbox swap verify" "$o" | head -2 >> results.txt
        grep "EXCEPTION\|FAIL\|refused" "$o" | head -3 >> results.txt
    done
done
echo done >> results.txt
cat results.txt
