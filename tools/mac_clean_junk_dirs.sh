#!/bin/sh
# Removes the empty junk folders an old build of the Mac game made next to the maps: progress_bar_initialize
# read the empty map directory past its end and made a folder named "z" and whatever bytes followed it
# in the data root ("z", "z%86", "z%FF%D0%BF%FF%D1", "zo", "z{" ...).
#
# usage: sh tools/mac_clean_junk_dirs.sh [--dry-run] [folder ...]
# With no folder it looks in the Halo.app's Resources, GameData and GameData/Maps (and the same under
# the folder named by HALO_MAC_DATA_ROOT, if set). Only the folders directly in each one are looked at,
# and only the ones whose name starts with z and that are EMPTY are removed, with rmdir: it never
# removes a file or a folder with anything in it, and never follows a link.
dry=0
if [ "$1" = "--dry-run" ]; then dry=1; shift; fi
if [ "$#" -eq 0 ]; then
    app=/Applications/Halo.app/Contents/Resources
    set -- "$app" "$app/GameData" "$app/GameData/Maps"
    if [ -n "${HALO_MAC_DATA_ROOT:-}" ]; then set -- "$@" "$HALO_MAC_DATA_ROOT" "$HALO_MAC_DATA_ROOT/Maps"; fi
fi
count=0
for folder in "$@"; do
    [ -d "$folder" ] || continue
    # (the names can hold any byte: find hands each to rmdir by itself)
    for found in "$folder"/z*; do
        [ -d "$found" ] || continue
        [ -L "$found" ] && continue
        # (an empty one: rmdir refuses the others)
        if [ "$(ls -A "$found" 2>/dev/null | wc -l | tr -d ' ')" != "0" ]; then continue; fi
        if [ "$dry" = 1 ]; then
            echo "would remove: $found"
            count=$((count + 1))
        elif rmdir "$found" 2>/dev/null; then
            echo "removed: $found"
            count=$((count + 1))
        fi
    done
done
echo "$count empty junk folder(s) $( [ "$dry" = 1 ] && echo 'found' || echo 'removed')"
