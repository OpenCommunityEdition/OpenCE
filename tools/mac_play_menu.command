#!/bin/sh
set -eu

launcher_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
binary="$launcher_dir/OpenCE-BloodGulch"
data_root=${HALO_MAC_DATA_ROOT:-/Applications/Halo.app/Contents/Resources/GameData}
state_root="$HOME/Library/Application Support/OpenCE PPC Test"
# the menus' pictures: the "menus" folder beside the binary (ce/shell/...)

if [ ! -x "$binary" ]; then
    echo "OpenCE-BloodGulch must be beside this launcher."
    exit 1
fi
for map in bloodgulch bitmaps sounds; do
    if [ ! -f "$data_root/Maps/$map.map" ]; then
        echo "Missing Halo map: $data_root/Maps/$map.map"
        exit 1
    fi
done

mkdir -p "$state_root/saves"
cd "$state_root"
export HALO_MAC_DATA_ROOT="$data_root"
export HALO_MAC_SAVE_ROOT="$state_root/saves"
# Use the combination verified by the native rendered-session checks. Keep
# explicit overrides available for comparisons on other hardware. The cache
# runtime uses an absent variable for its smaller default; accept 0 here.
export HALO_START_IN_MENU=1
export HALO_MAC_BONE_GATHER=${HALO_MAC_BONE_GATHER:-1}
case ${HALO_MAC_RETAIL_TEXTURE_CACHE_16M:-1} in
    0) unset HALO_MAC_RETAIL_TEXTURE_CACHE_16M ;;
    1) export HALO_MAC_RETAIL_TEXTURE_CACHE_16M=1 ;;
    *) echo "HALO_MAC_RETAIL_TEXTURE_CACHE_16M must be 0 or 1." >&2; exit 2 ;;
esac
unset HALO_MAC_RENDER_FRAMES HALO_MAC_RENDER_TRACE HALO_MAC_SESSION_TRACE
unset HALO_MAC_RENDER_FIXED_STEP HALO_MAC_RENDER_SEED
echo "Loading the experimental OpenCE build with the PC version's menus. This may take a few minutes."
echo "Menus: arrow keys, Return to choose, Backspace to go back. In a game: WASD, mouse look, Escape for the pause menu."
echo "Quit is in the main menu, or close the window."
exec "$binary" "$data_root/Maps/bloodgulch.map" \
    "$data_root/Maps/bitmaps.map" "$data_root/Maps/sounds.map" --gpu-play
