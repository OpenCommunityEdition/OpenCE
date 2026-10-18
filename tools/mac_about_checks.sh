#!/bin/sh
# Checks PowerSpark's About and Send Feedback windows on the Mac that builds the app (run in the repository's
# root): the report code's host test, the launcher built for ppc and i386 into a bundle of just its own parts,
# and its layout report (--about-dump: each control's rectangle, its text's size in the system font, and a
# nonzero exit when something does not fit), once without branding (the repository's neutral build) and once
# with BRANDING=FILE (a branding.plist as tools/mac_app_bundle.sh --branding takes, with its files beside it).
# --about-selftest makes both windows without showing them (it needs the window server) and reads the
# controls back. With BRANDING set and PowerSpark-send built (SENDER=FILE), the selftest also sends one report
# marked "PowerSpark-test" to the server in FEEDBACK_TEST_URL, with the CA list FEEDBACK_TEST_CA (a server of
# your own: it never touches the real endpoint).
#
#   sh tools/mac_about_checks.sh [BRANDING=FILE] [SENDER=FILE] [FEEDBACK_TEST_URL=URL FEEDBACK_TEST_CA=FILE]
set -eu
for arg in "$@"; do
    case $arg in
        BRANDING=*|SENDER=*|FEEDBACK_TEST_URL=*|FEEDBACK_TEST_CA=*) eval "$arg" ;;
        *) echo "unknown argument $arg" >&2; exit 2 ;;
    esac
done
: "${BRANDING:=}" "${SENDER:=}" "${FEEDBACK_TEST_URL:=}" "${FEEDBACK_TEST_CA:=}"
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_CHECK_DIR:-build/mac-about}
app=port/mac/app
toml=port/third_party/tomlc17
rm -rf "$out/PowerSpark.app" "$out/test_home"
mkdir -p "$out/PowerSpark.app/Contents/MacOS" "$out/PowerSpark.app/Contents/Resources"
flags="-arch ppc -arch i386 -isysroot $mac_sdk -mmacosx-version-min=10.4"

"$mac_cc" -std=gnu89 -Wall -o "$out/test_feedback_report" "$app/test_feedback_report.c" "$app/feedback_report.c"
"$out/test_feedback_report" "$out/test_home"

"$mac_cc" $flags -O2 -std=gnu99 -include "$app/mac_toml_assert.h" -I"$app" -I"$toml" -c "$app/launcher_toml_check.c" -o "$out/launcher_toml_check.o"
"$mac_cc" $flags -O2 -std=gnu99 -include "$app/mac_toml_assert.h" -I"$toml" -c "$toml/tomlc17.c" -o "$out/tomlc17.o"
"$mac_cc" $flags -O2 -Wall -std=gnu89 -o "$out/PowerSpark.app/Contents/MacOS/PowerSpark" "$app/opence_launcher.c" \
    "$app/launcher_config.c" "$app/launcher_dialog.c" "$app/launcher_about.c" "$app/launcher_branding.c" \
    "$app/feedback_report.c" "$out/launcher_toml_check.o" "$out/tomlc17.o" -framework Carbon -framework CoreFoundation
sed -e 's/@VERSION@/0.1/' -e 's/@BUILD@/check/' "$app/Info.plist" > "$out/PowerSpark.app/Contents/Info.plist"
cp "$app/OpenCE.icns" "$out/PowerSpark.app/Contents/Resources/PowerSpark.icns"
launcher=$out/PowerSpark.app/Contents/MacOS/PowerSpark

echo "== neutral build (no branding.plist)"
"$launcher" --about-dump
"$launcher" --about-selftest
if [ -n "$BRANDING" ]; then
    echo "== branding $BRANDING"
    plutil -lint "$BRANDING"
    cp "$(dirname "$BRANDING")"/* "$out/PowerSpark.app/Contents/Resources/"
    cp "$BRANDING" "$out/PowerSpark.app/Contents/Resources/branding.plist"
    if [ -n "$SENDER" ]; then cp "$SENDER" "$out/PowerSpark.app/Contents/MacOS/PowerSpark-send"; fi
    "$launcher" --about-dump
    if [ -n "$FEEDBACK_TEST_URL" ]; then
        HOME_DIR=$(pwd)/$out/test_home
        HALO_MAC_FEEDBACK_TEST_URL=$FEEDBACK_TEST_URL HALO_MAC_FEEDBACK_TEST_CA=$FEEDBACK_TEST_CA \
            HALO_MAC_FEEDBACK_TEST_HOME=$HOME_DIR "$launcher" --about-selftest
    else
        "$launcher" --about-selftest
    fi
fi
echo "about checks: ok"
