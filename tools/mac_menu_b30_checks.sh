#!/bin/sh
# The Silent Cartographer entered through the menus (the title map, Campaign, Load Level), not opened directly:
# its opening cutscene must play to its end, the player landing on the beach at about (-61.4, 38.9) by tick 1200.
# (Seen once as a Pelican left in the sea on a PowerBook: the direct b30 gates did not cover the menu path.)
# usage: sh tools/mac_menu_b30_checks.sh SESSION_BINARY DATA_ROOT UI_MAP OUTDIR [more maps for the retail form]
#   Xbox maps: DATA_ROOT=.../xbox ; UI_MAP=$DATA_ROOT/maps/ui.map
#   retail:    DATA_ROOT=.../GameData ; UI_MAP=$DATA_ROOT/Maps/ui.map BITMAPS SOUNDS
# HALO_MAC_MENU_ART_DIR must name port/assets/menus outside a kit. HALO_MAC_FRAME_SLOW_MS (with FIXED=0) plays a
# slow Mac's several ticks a frame. Prints "checks, N failures" like the other checks.
set -u
binary=$1; root=$2; ui=$3; out=$4; shift 4
mkdir -p "$out/save" "$out/cache"; rm -f "$out/out.txt"
wait8=wait,wait,wait,wait,wait,wait,wait,wait,
long=""; i=0; while [ $i -lt 300 ]; do long="${long}wait,"; i=$((i+1)); done
cd "$out"
env HALO_MAC_MENU=1 HALO_START_IN_MENU=1 HALO_MAC_RENDER_FIXED_STEP=${FIXED:-1} HALO_MAC_RENDER_SEED=1835103336 \
    HALO_MAC_RENDER_FRAMES=${FRAMES:-2600} HALO_MAC_DATA_ROOT="$root" HALO_MAC_MAP_CACHE="$out/cache" \
    HALO_MAC_SAVE_ROOT="$out/save" HALO_MAC_CONFIG_DIR="$out/cfg" HALO_MAC_RENDER_CAPTURE="$out/frame.ppm" \
    HALO_MAC_CUTSCENE_LOG=100 HALO_MAC_MENU_KEY_FRAMES=4 \
    HALO_MAC_MENU_KEYS="${wait8}return,${wait8}return,${wait8}down,down,down,wait,wait,return,${wait8}return,${long}end" \
    "$binary" "$ui" "$@" --gpu-render > "$out/out.txt" 2>&1
status=$?
checks=0; failures=0
check() { checks=$((checks+1)); if [ "$1" != 1 ]; then failures=$((failures+1)); echo "FAIL: $2"; fi; }
check $([ $status = 0 ] && echo 1 || echo 0) "the run exits 0 (exit $status)"
check $(grep -a -q "menus: the game levels.b30.b30 is started" out.txt && echo 1 || echo 0) "the menus start b30"
# the b30 session's cutscene lines are those after the start line
after=$(awk '/menus: the game levels.b30.b30 is started/{f=1} f&&/^cutscene tick/' out.txt)
echo "$after" | grep -q "cinematic=1" ; check $([ $? = 0 ] && echo 1 || echo 0) "the opening cinematic runs"
last=$(echo "$after" | tail -1)
echo "$last" | grep -q "cinematic=0"; check $([ $? = 0 ] && echo 1 || echo 0) "the cinematic ends"
landed=$(echo "$last" | sed -n 's/.*player=(\([-0-9.]*\),\([-0-9.]*\),.*/\1 \2/p')
ok=$(echo "$landed" | awk '{ x=$1+61.4; y=$2-38.9; print (x*x+y*y < 9) ? 1 : 0 }')
check "${ok:-0}" "the player is on the beach near (-61.4, 38.9) (got: $landed)"
echo "menu b30 opening: $checks checks, $failures failures"
[ $failures = 0 ]
