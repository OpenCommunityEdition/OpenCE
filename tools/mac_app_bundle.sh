#!/bin/sh
# Assembles PowerSpark.app (ppc + i386, Mac OS X 10.4 and later) and a disk image.
# Run on the Intel Mac that builds the port (Xcode 3.x, gcc 4.0, the 10.4u
# SDK), in the repository's root.
#
#   sh tools/mac_app_bundle.sh --engine PATH | --engine-ppc PATH --engine-i386 PATH
#       [--helpers DIR] [--driver PKG] [--icon FILE] [--branding FILE] [--out DIR] [--version X.Y] [--no-dmg]
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
#   --branding     a branding.plist for the About and Send Feedback windows, kept out of the
#                  repository like the icon (see port/mac/README.md, "About and feedback"):
#                  PublisherText, Links (Title and URL), LogoFile and CAFile (names of files
#                  beside the plist, which are copied in), FeedbackURL, FeedbackApp,
#                  FeedbackToken, UpdateFeedURL, UpdatePublicKey. Without it the About window has
#                  the OpenCE link only, there is no Send Feedback and the app does not look for
#                  updates. With a FeedbackURL (and a CAFile), PowerSpark-send, the program that posts a
#                  report with mbedtls, is built and added. With an UpdateFeedURL (https; the appcast of
#                  tools/powerspark_sign_update.py), a CAFile and an UpdatePublicKey (the release key's
#                  public half, base64), PowerSpark-update, the program that downloads updates, is
#                  built and added (port/mac/README.md, "Updates").
#   --out          the output folder (default build/mac-app)
#
# Writes OUT/PowerSpark.app and, unless --no-dmg, OUT/PowerSpark-VERSION.dmg.
set -eu

ROOT=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
CC=${CC:-gcc-4.0}
SDK=${SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
OUT=$ROOT/build/mac-app
VERSION=0.1
ENGINE= ENGINE_PPC= ENGINE_I386= HELPERS= DRIVER= ICON= BRANDING= DMG=1
while [ $# -gt 0 ]; do
    case $1 in
        --engine) ENGINE=$2; shift ;;
        --engine-ppc) ENGINE_PPC=$2; shift ;;
        --engine-i386) ENGINE_I386=$2; shift ;;
        --helpers) HELPERS=$2; shift ;;
        --driver) DRIVER=$2; shift ;;
        --icon) ICON=$2; shift ;;
        --branding) BRANDING=$2; shift ;;
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
# (the feedback report's JSON, log and Outbox code is tested the same way)
$CC -std=gnu89 -Wall -o "$WORK/test_feedback_report" "$APPSRC/test_feedback_report.c" "$APPSRC/feedback_report.c"
"$WORK/test_feedback_report" "$WORK/feedback_test"
# (what the launcher does with a halo:// invite link, which needs no Mac)
mkdir -p "$WORK/invite_test"
$CC -std=gnu89 -Wall -o "$WORK/test_launcher_invite" "$APPSRC/test_launcher_invite.c" "$APPSRC/launcher_invite.c"
"$WORK/test_launcher_invite" "$WORK/invite_test"
# (the updater's appcast, signature, SHA-256, bundle and swap code is tested the same way; its Ed25519 is Monocypher's)
MONO=$ROOT/port/third_party/monocypher
$CC -std=gnu99 -Wall -I"$MONO" -I"$APPSRC" -o "$WORK/test_update_core" "$APPSRC/test_update_core.c" "$APPSRC/update_core.c" \
    "$MONO/monocypher.c" "$MONO/monocypher-ed25519.c" -framework CoreFoundation
"$WORK/test_update_core" "$WORK/update_test"
for src in monocypher monocypher-ed25519; do
    $CC -arch ppc -arch i386 -isysroot "$SDK" -mmacosx-version-min=10.4 -O2 -std=gnu99 -I"$MONO" -c "$MONO/$src.c" -o "$WORK/$src.o"
done
$CC -arch ppc -arch i386 -isysroot "$SDK" -mmacosx-version-min=10.4 -O2 -Wall -std=gnu89 -I"$MONO" \
    -o "$APP/Contents/MacOS/PowerSpark" "$APPSRC/opence_launcher.c" "$APPSRC/launcher_config.c" \
    "$APPSRC/launcher_dialog.c" "$APPSRC/launcher_about.c" "$APPSRC/launcher_branding.c" "$APPSRC/feedback_report.c" \
    "$APPSRC/launcher_invite.c" "$APPSRC/launcher_update.c" "$APPSRC/update_core.c" \
    "$WORK/launcher_toml_check.o" "$WORK/tomlc17.o" "$WORK/monocypher.o" "$WORK/monocypher-ed25519.o" \
    -framework Carbon -framework CoreFoundation

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

# the About and Send Feedback windows' branding (--branding): the plist and the files it names
if [ -n "$BRANDING" ]; then
    [ -f "$BRANDING" ] || { echo "no branding file $BRANDING" >&2; exit 2; }
    BRANDDIR=$(CDPATH= cd "$(dirname "$BRANDING")" && pwd)
    plutil -lint "$BRANDING" > /dev/null
    cp "$BRANDING" "$APP/Contents/Resources/branding.plist"
    brand_key() { /usr/libexec/PlistBuddy -c "Print :$1" "$BRANDING" 2> /dev/null || true; }
    for key in LogoFile CAFile; do
        name=$(brand_key $key)
        case $name in
            "") ;;
            */*|.*) echo "branding: $key must be a plain file name beside the plist" >&2; exit 2 ;;
            *) [ -f "$BRANDDIR/$name" ] || { echo "branding: no $BRANDDIR/$name" >&2; exit 2; }
               cp "$BRANDDIR/$name" "$APP/Contents/Resources/$name" ;;
        esac
    done
    FEEDBACK_URL=$(brand_key FeedbackURL)
    UPDATE_URL=$(brand_key UpdateFeedURL)
    UPDATE_KEY=$(brand_key UpdatePublicKey)
    if [ -n "$FEEDBACK_URL" ]; then
        [ -n "$(brand_key CAFile)" ] || { echo "branding: FeedbackURL needs a CAFile (the CA list the HTTPS is checked with)" >&2; exit 2; }
    fi
    if [ -n "$UPDATE_URL" ] || [ -n "$UPDATE_KEY" ]; then
        [ -n "$(brand_key CAFile)" ] || { echo "branding: UpdateFeedURL needs a CAFile (the CA list the HTTPS is checked with)" >&2; exit 2; }
        case $UPDATE_URL in https://*) ;; *) echo "branding: UpdateFeedURL must be an https address" >&2; exit 2 ;; esac
        # (32 bytes in base64: 44 characters, the last of them padding)
        case $UPDATE_KEY in
            ????????????????????????????????????????????) ;;
            *) echo "branding: UpdatePublicKey must be the release key's 32 bytes in base64 (tools/powerspark_sign_update.py keygen)" >&2; exit 2 ;;
        esac
        case $UPDATE_KEY in *=) ;; *) echo "branding: UpdatePublicKey is not 32 bytes in base64" >&2; exit 2 ;; esac
    fi
    if [ -n "$FEEDBACK_URL" ] || [ -n "$UPDATE_URL" ]; then
        # mbedtls (the vendored copy, with the Mac's configuration) for PowerSpark-send and PowerSpark-update. The
        # library's objects are kept in build/mac-feedback and rebuilt when a source is newer, since they take a while
        MB=$ROOT/port/third_party/mbedtls
        OBJ=$ROOT/build/mac-feedback
        mkdir -p "$OBJ"
        for src in "$MB"/library/*.c; do
            o=$OBJ/$(basename "$src" .c).o
            if [ ! -f "$o" ] || [ "$src" -nt "$o" ] || [ "$ROOT/port/mac/mac_mbedtls_config.h" -nt "$o" ]; then
                $CC -arch ppc -arch i386 -isysroot "$SDK" -mmacosx-version-min=10.4 -std=gnu99 -Os \
                    -DMBEDTLS_CONFIG_FILE='"mac_mbedtls_config.h"' -ffunction-sections -fdata-sections \
                    -I"$ROOT/port/mac" -I"$MB/include" -I"$MB/library" -c "$src" -o "$o"
            fi
        done
    fi
    if [ -n "$FEEDBACK_URL" ]; then
        # PowerSpark-send: the sender of reports
        $CC -arch ppc -arch i386 -isysroot "$SDK" -mmacosx-version-min=10.4 -std=gnu99 -Os -Wall \
            -DMBEDTLS_CONFIG_FILE='"mac_mbedtls_config.h"' -ffunction-sections -fdata-sections \
            -I"$ROOT/port/mac" -I"$MB/include" -o "$APP/Contents/MacOS/PowerSpark-send" "$APPSRC/feedback_send.c" \
            "$ROOT/port/mac/mac_mbedtls_time.c" "$OBJ"/*.o -framework CoreServices -Wl,-dead_strip
        chmod 755 "$APP/Contents/MacOS/PowerSpark-send"
    fi
    if [ -n "$UPDATE_URL" ]; then
        # PowerSpark-update: the downloader of the appcast and of releases
        $CC -arch ppc -arch i386 -isysroot "$SDK" -mmacosx-version-min=10.4 -std=gnu99 -Os -Wall \
            -DMBEDTLS_CONFIG_FILE='"mac_mbedtls_config.h"' -ffunction-sections -fdata-sections \
            -I"$ROOT/port/mac" -I"$MB/include" -o "$APP/Contents/MacOS/PowerSpark-update" "$APPSRC/update_fetch.c" \
            "$ROOT/port/mac/mac_mbedtls_time.c" "$OBJ"/*.o -framework CoreServices -Wl,-dead_strip
        chmod 755 "$APP/Contents/MacOS/PowerSpark-update"
    fi
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
if [ -f "$APP/Contents/MacOS/PowerSpark-send" ]; then lipo -info "$APP/Contents/MacOS/PowerSpark-send"; fi
if [ -f "$APP/Contents/MacOS/PowerSpark-update" ]; then lipo -info "$APP/Contents/MacOS/PowerSpark-update"; fi

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
