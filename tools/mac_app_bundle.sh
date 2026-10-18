#!/bin/sh
# Assembles PowerSpark.app (ppc + i386, Mac OS X 10.4 and later) and a disk image.
# Run on the Intel Mac that builds the port (Xcode 3.x, gcc 4.0, the 10.4u
# SDK), in the repository's root.
#
#   sh tools/mac_app_bundle.sh --engine PATH | --engine-ppc PATH --engine-i386 PATH
#       [--helpers DIR] [--driver PKG] [--icon FILE] [--out DIR] [--version X.Y] [--no-dmg]
#
#   --engine       the game as one universal program, or
#   --engine-ppc / --engine-i386   its two builds (joined with lipo): the
#                  mac_rendered_session_ppc and _i386 that
#                  tools/mac_rendered_session_checks.sh writes
#   --helpers      the folder holding "OpenCE Setup.app" and "OpenCE
#                  Controller Setup.app" (port/mac/xiso_setup/build.sh and
#                  port/mac/controller_setup/build.sh write them); they are
#                  built here when it is not given
#   --driver       ignored (older scripts pass it): the app no longer carries
#                  a controller driver; Controller Setup tells players to use
#                  the 8BitDo Wireless USB Adapter 2 instead
#   --icon         the app's icon, a .icns or a square .png (made into one by
#                  tools/mac_icns.py); kept out of the repository (PowerSpark's
#                  is fan art, not OpenCE's to ship). Without it the app has
#                  OpenCE's own icon (port/mac/app/OpenCE.icns).
#   --out          the output folder (default build/mac-app)
#
# Writes OUT/PowerSpark.app and, unless --no-dmg, OUT/PowerSpark-VERSION.dmg.
set -eu

ROOT=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
CC=${CC:-gcc-4.0}
SDK=${SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
OUT=$ROOT/build/mac-app
VERSION=0.1
ENGINE= ENGINE_PPC= ENGINE_I386= HELPERS= DRIVER= ICON= DMG=1
while [ $# -gt 0 ]; do
    case $1 in
        --engine) ENGINE=$2; shift ;;
        --engine-ppc) ENGINE_PPC=$2; shift ;;
        --engine-i386) ENGINE_I386=$2; shift ;;
        --helpers) HELPERS=$2; shift ;;
        --driver) DRIVER=$2; shift ;;
        --icon) ICON=$2; shift ;;
        --out) OUT=$2; shift ;;
        --version) VERSION=$2; shift ;;
        --no-dmg) DMG=0 ;;
        *) echo "unknown option $1" >&2; exit 2 ;;
    esac
    shift
done
if [ -z "$ENGINE" ] && { [ -z "$ENGINE_PPC" ] || [ -z "$ENGINE_I386" ]; }; then
    echo "give --engine, or --engine-ppc and --engine-i386" >&2
    exit 2
fi

APPSRC=$ROOT/port/mac/app
APP=$OUT/PowerSpark.app
WORK=$OUT/work
rm -rf "$APP" "$WORK"
mkdir -p "$WORK" "$APP/Contents/MacOS" "$APP/Contents/Resources/Helpers"

# the launcher (its settings file code is tested first, on this Mac)
$CC -std=gnu89 -Wall -o "$WORK/test_launcher_config" "$APPSRC/test_launcher_config.c" "$APPSRC/launcher_config.c"
"$WORK/test_launcher_config" "$WORK/launcher_test.toml"
# (the settings file's repair and check, with the vendored TOML parser the game reads it with; GCC 4 needs the shim for static_assert)
TOML=$ROOT/port/third_party/tomlc17
$CC -std=gnu99 -Wall -include "$APPSRC/mac_toml_assert.h" -I"$TOML" -o "$WORK/test_launcher_toml" \
    "$APPSRC/test_launcher_toml.c" "$APPSRC/launcher_config.c" "$APPSRC/launcher_toml_check.c" "$TOML/tomlc17.c"
"$WORK/test_launcher_toml"
$CC -arch ppc -arch i386 -isysroot "$SDK" -mmacosx-version-min=10.4 -O2 -std=gnu99 -include "$APPSRC/mac_toml_assert.h" \
    -I"$APPSRC" -I"$TOML" -c "$APPSRC/launcher_toml_check.c" -o "$WORK/launcher_toml_check.o"
$CC -arch ppc -arch i386 -isysroot "$SDK" -mmacosx-version-min=10.4 -O2 -std=gnu99 -include "$APPSRC/mac_toml_assert.h" \
    -I"$TOML" -c "$TOML/tomlc17.c" -o "$WORK/tomlc17.o"
# (the startup dialog's sources are beside it: launcher_config.c is plain C, launcher_dialog.c Carbon)
$CC -arch ppc -arch i386 -isysroot "$SDK" -mmacosx-version-min=10.4 -O2 -Wall -std=gnu89 \
    -o "$APP/Contents/MacOS/PowerSpark" "$APPSRC/opence_launcher.c" "$APPSRC/launcher_config.c" \
    "$APPSRC/launcher_dialog.c" "$WORK/launcher_toml_check.o" "$WORK/tomlc17.o" -framework Carbon -framework CoreFoundation

# the game
if [ -n "$ENGINE" ]; then
    cp "$ENGINE" "$APP/Contents/MacOS/PowerSpark-game"
else
    lipo -create "$ENGINE_PPC" "$ENGINE_I386" -output "$APP/Contents/MacOS/PowerSpark-game"
fi
chmod 755 "$APP/Contents/MacOS/PowerSpark" "$APP/Contents/MacOS/PowerSpark-game"

# what the game program supports, for the launcher (engine.txt)
: > "$APP/Contents/Resources/engine.txt"
if grep -q HALO_MAC_CONFIG_DIR "$APP/Contents/MacOS/PowerSpark-game"; then
    echo config_dir >> "$APP/Contents/Resources/engine.txt"
fi

# the menus' pictures, found by the launcher (HALO_MAC_MENU_ART_DIR)
cp -R "$ROOT/port/assets/menus" "$APP/Contents/Resources/menus"
find "$APP" -name .DS_Store -exec rm -f {} \;

# the helper apps
if [ -z "$HELPERS" ]; then
    HELPERS=$WORK/helpers
    mkdir -p "$HELPERS"
    ( cd "$ROOT/port/mac/xiso_setup" && sh build.sh && rm -rf "$HELPERS/OpenCE Setup.app" \
        && mv "OpenCE Setup.app" "$HELPERS/" && rm -f xiso_cli )
    ( cd "$ROOT/port/mac/controller_setup" && sh build.sh && rm -rf "$HELPERS/OpenCE Controller Setup.app" \
        && mv "OpenCE Controller Setup.app" "$HELPERS/" )
fi
for h in "OpenCE Setup.app" "OpenCE Controller Setup.app"; do
    cp -R "$HELPERS/$h" "$APP/Contents/Resources/Helpers/$h"
done
if [ -n "$DRIVER" ]; then
    echo "mac_app_bundle: --driver is ignored; the app carries no controller driver" >&2
fi

# the icon: --icon's (.icns as it is, .png through tools/mac_icns.py), else OpenCE's
case $ICON in
    *.icns) cp "$ICON" "$APP/Contents/Resources/PowerSpark.icns" ;;
    ?*) python3 "$ROOT/tools/mac_icns.py" "$ICON" "$APP/Contents/Resources/PowerSpark.icns" ;;
    *) cp "$APPSRC/OpenCE.icns" "$APP/Contents/Resources/PowerSpark.icns" ;;
esac
BUILD=$(date +%Y%m%d)
sed -e "s/@VERSION@/$VERSION/" -e "s/@BUILD@/$BUILD/" "$APPSRC/Info.plist" > "$APP/Contents/Info.plist"
printf 'APPL????' > "$APP/Contents/PkgInfo"
plutil -lint "$APP/Contents/Info.plist"
lipo -info "$APP/Contents/MacOS/PowerSpark" "$APP/Contents/MacOS/PowerSpark-game"

if [ "$DMG" = 1 ]; then
    DMGDIR=$WORK/dmg
    mkdir -p "$DMGDIR"
    COPYFILE_DISABLE=1 cp -R "$APP" "$DMGDIR/PowerSpark.app"
    if [ -f "$ROOT/port/mac/README.md" ]; then cp "$ROOT/port/mac/README.md" "$DMGDIR/READ ME.txt"; fi
    rm -f "$OUT/PowerSpark-$VERSION.dmg"
    hdiutil create -quiet -srcfolder "$DMGDIR" -volname PowerSpark -format UDZO -ov "$OUT/PowerSpark-$VERSION.dmg"
    ls -l "$OUT/PowerSpark-$VERSION.dmg"
fi
rm -rf "$WORK"
echo "built $APP"
