# MCC package images in the menus

MCC map and mission panels in the stock OpenCE menu display the image declared
by their installed package, retaining the existing thumbnail rectangles and typography. All MCC images are center-cropped and
resized to fill the same 140 by 114 logical-pixel area as the question-mark
placeholder, without stretching. An explicit clip prevents drawing outside
that rectangle. Cropping happens while preparing the texture: the native
bitmap helper samples from the texture origin and cannot select an offset crop.
The lobby's locally installed map uses the same provider.

This is interpretation of the selected MCC package, with no additional toggle.
Existing Steam discovery and campaign settings still control discovery. Xbox
and Custom Edition keep their own picture providers and loading paths.

## Prerequisites and one example

Keep the package's `ModInfo.json`, listed map/mission JSON files and image
files together. Multiplayer JSON is listed in `GameModContents.MultiplayerMaps`;
campaign JSON in `CampaignInfo.json`'s `CampaignMaps`. The selected level must
match exactly one listed `ScenarioFile` basename. The image resolver tries
`Images.Large`, then `Images.Thumbnail`. A missing, unsupported or damaged
image leaves the normal question-mark picture.

Multiplayer lists support up to 1,024 entries, matching the Workshop catalog;
campaign lists retain their separate 64-mission bound. These are reader
capacities, not authored format limits. The item path buffer and per-preview
metadata are allocated on the heap and released on success or failure, so a
large collection does not grow the preview-loading stack. Catalog label
metadata remains in its existing static package/family cache.

For example, a mission declaring `Images.Large: "assets/1.png"` reads that
file from its own package root. A map declaring `"frogfoot_mid.png"` reads
from the root itself. There is no guessed filename or recursive image search.
A local `mcc_maps` override resolves its own metadata even if its folder has
the same Workshop ID as a Steam installation.

Android's folder and ZIP importer already retains PNG files and metadata
subdirectories, while flattening the maps into the package root. Preview lookup
is case-insensitive because imports normalize destination names. Steam is not
required. Import the complete package again if an older manual copy contains
only the `.map` files.

The same local metadata supplies package and level titles without Steam.
Workshop IDs remain part of the internal package identity, not a suffix on
the level label. A numeric package with no usable title shows "MCC package";
named manual packages retain their established filename fallback. No on-disk
folder, saved mission identity or network map name is renamed.

## Reading route and boundaries

1. [menu_functions.c](../../port/linux/game/menu_functions.c):
   `mcc_map_description_show` and `lobby_map_show` supply MCC display indices.
2. [mcc_manifest.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/src/mcc_manifest.c):
   `mcc_manifest_package_root` follows the actual existing MCC path mapping.
3. [mcc_menu_metadata.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/src/mcc_menu_metadata.c):
   `mcc_menu_metadata_load` reads the explicit map list and
   `mcc_menu_metadata_find` refuses ambiguous basenames. The catalog reads each
   package/family once per scan for its labels.
4. [mcc_preview_image.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/src/mcc_preview_image.c):
   `mcc_preview_image_load` uses that metadata; `mcc_preview_png` decodes
   bounded PNG bytes. `mcc_preview_image_free` releases the platform allocation
   in its allocating module, rather than through the game's debug allocator.
5. [posix_mcc_preview.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/src/posix_mcc_preview.c):
   `posix_mcc_preview_read` performs bounded, regular-file reads below the
   selected root, rejecting relative-path escapes, devices and linked child
   paths. POSIX uses held directory descriptors; Windows checks reparse points
   and the opened file's final path. This file runs in the Android host through
   the generated `posix.h` guest bridge.
6. [mcc_menu_preview.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_menu_preview.c):
   `mcc_menu_preview_draw` owns texture caching, centered cropping and clipping.
7. [mcc_menu_text.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_menu_text.c):
   `mcc_menu_text_draw` measures package text in the active font, wraps it at
   word boundaries and breaks oversized words. It uses the existing small font
   when necessary and an ellipsis when the fixed panel still cannot fit it.
   MCC-only PC list callbacks also use `mcc_menu_text_row` to fit a single-line
   label beside the navigation arrows; the catalog title itself is unchanged.

The minimal hooks in [ui_widget.c](../../source/interface/ui_widget.c) are
necessary because the shared widget renderer otherwise treats a picture's
display index as a frame in the loaded tag and draws text without MCC-aware
wrapping. Only MCC indices on the level-picture tag enter the new image path;
the original frame is the fallback. Text hooks accept MCC catalog fields and
dedicated MCC folder fields. Neither hook repurposes the Xbox/CE providers.

## Stock campaign and multiplayer flow

The stock selector offers separate MCC singleplayer and multiplayer categories.
Campaigns require `game.mcc_campaigns = true` (the default). Package folders open
into mission lists; Back leaves the difficulty step, then the package, then the selector.
The host selects a mission and difficulty through the existing hosting flow.
The MCC catalog owns these identities; no MCC mission becomes an Xbox level index.
The same generated menu and callbacks run on native and browser builds.

## MCC completion badges

[mcc_progress.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_progress.c) stores completion in
`mcc_completed.dat` beside each player's native profile. Records identify the
full `mcc_maps\\package\\mission` path, so equally named missions in different
packages and different profiles do not share awards. Normal, Heroic and Legendary
use the stock selector's existing difficulty indicators. Easy is
recorded but has no indicator, matching the stock selector. Higher difficulty
does not invent awards for lower difficulties. Existing playthroughs cannot be
reconstructed into completion records; awards begin with this version.

`mcc_map_description_show` reads the selected profile and full mission identity.
`mcc_progress_menu_flags` caches one selection for at most one second to avoid
reading the journal every frame; profile or mission changes bypass the timer.
Folder and multiplayer entries show no completion badges.

The existing MCC campaign-win hook records actual completion before advancing
or returning to the menu. It follows the existing native authority model: local
players on a solo/couch-co-op game or the host receive records; remote clients
do not infer completion from a map change. No new network messages are added.
The journal is append-only, limited to 4,096 records of 76 bytes, with duplicate
awards suppressed. Version, bounded mission identity, flags and checksum are
checked. A damaged tail preserves earlier readable awards and blocks further
appends; it is not silently replaced. A checksum detects corruption, not cheating.

Profile rename normally copies only native save files before deleting the old
directory. One guarded call in
[saved_game_files.c](../../source/saved%20games/saved_game_files.c) therefore
copies the MCC companion through `mcc_progress_copy` before deletion. An absent
companion leaves native behavior intact; a failed copy preserves the old profile.
This lifecycle boundary cannot be handled solely when awarding/displaying badges.

## Ownership, limits and checks

The four-entry cache is lazy, includes failed reads, and is cleared by
`mcc_maps_rescan` and `menu_tags_unloaded`. Each texture is at most 512 by 512;
its power-of-two padding is excluded from the sampled rectangle. Decoded source
pixels are freed immediately after upload. Reopening a list permits changed
images to be retried; loading/unloading maps cannot retain old menu textures.

Supported images are non-interlaced, 8-bit RGB or RGBA PNGs, at most 2048 pixels
per dimension and 8 MiB on disk. Other PNG encodings and other image formats
fall back rather than being guessed. Chunk lengths, CRCs, ordering, filters and
the exact inflated byte count are checked. Image metadata does not request
network downloads, execute code, or change MCC redistribution/map-sharing policy.
The decoder was independently implemented against the
[PNG specification](https://www.w3.org/TR/png-3/).

[test_mcc_previews.py](https://github.com/chestahh/GulchCE/blob/1db929c5e34101a45f644436e031016b5584c7bf/tools/test_mcc_previews.py) executes the real
decoder and file reader with generated assets, including all five PNG filters,
malformed files, path/link escapes, metadata fallback and local precedence.
[test_mcc_preview_menu.py](https://github.com/chestahh/GulchCE/blob/1db929c5e34101a45f644436e031016b5584c7bf/tools/test_mcc_preview_menu.py) exercises the
real presentation against deterministic font/GPU services, checking bounds,
cache eviction/reset, wrapping and unaffected Xbox/CE indices. The
[Android importer tests](https://github.com/chestahh/GulchCE/blob/1db929c5e34101a45f644436e031016b5584c7bf/tools/tests/MccPackageImporterTest.java) verify
that folder and ZIP imports retain metadata and image bytes. The full source archive
ran preview reader checks on Windows and Linux, including Linux
address/undefined-behavior sanitizers. These tests are archived coverage;
they are not part of the focused testing-based contribution. These automated checks do not replace rendered menu inspection or
Android device testing.

[test_mcc_progress.py](https://github.com/chestahh/GulchCE/blob/1db929c5e34101a45f644436e031016b5584c7bf/tools/test_mcc_progress.py) executes journal reads
and writes, profile/mission isolation, duplicate awards, corruption, capacity
and profile-rename copying. The existing MCC lifecycle test checks that awards
are gated by MCC campaign activation and host/local authority.
