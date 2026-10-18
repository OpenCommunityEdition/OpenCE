#!/bin/sh
# The updater's scenarios, run on the Mac that tests PowerSpark (Mac OS X 10.4 to 10.6) by
# tools/mac_update_checks.py, which has put a fixture beside this script and tunnelled a local HTTPS test
# server (tools/mac_update_test_server.py) to localhost:18443 here:
#
#   sh mac_update_scenarios.sh FIXTURE_DIR WORK_DIR [ARCH...]     (ARCH: i386 and ppc by default)
#
# Each scenario runs PowerSpark.app (a scratch copy of the fixture's old app, version 0.1, in a folder of its own
# and with a home folder of its own) with HALO_MAC_UPDATE_HEADLESS=1 and says PASS or FAIL. Nothing of the real
# user's is touched: not the real home, not any installed PowerSpark.
FIX=$1
WORK=$2
shift 2
ARCHS=${*:-i386 ppc}
pass=0
fail=0

ok() { echo "PASS  $1"; pass=$((pass + 1)); }
bad() { echo "FAIL  $1"; fail=$((fail + 1)); [ -n "${S:-}" ] && [ -f "$S/out.txt" ] && sed 's/^/        | /' "$S/out.txt" | tail -12; }
check() { # check NAME COMMAND...   (the command succeeds)
    name=$1; shift
    if "$@" > /dev/null 2>&1; then ok "$name"; else bad "$name"; fi
}
version_of() { /usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$1/Contents/Info.plist" 2> /dev/null; }
has() { grep -q -- "$1" "$S/out.txt"; }
lacks() { ! grep -q -- "$1" "$S/out.txt"; }

# a scratch copy of the old app, and a home with a settings file and a saved game in it
new_case() {
    S=$WORK/$1
    chmod -R u+w "$S" 2> /dev/null
    rm -rf "$S"
    mkdir -p "$S/apps" "$S/home/Library/Application Support/OpenCE/saves"
    cp -R "$FIX/old/PowerSpark.app" "$S/apps/PowerSpark.app"
    APP=$S/apps/PowerSpark.app
    L=$APP/Contents/MacOS/PowerSpark
    SUPPORT="$S/home/Library/Application Support/OpenCE"
    printf '[display]\nmode = "windowed"\n' > "$SUPPORT/config.toml"
    echo "my saved game" > "$SUPPORT/saves/save1.bin"
    printf '#!/bin/sh\necho "$@" >> "%s/opened.txt"\n' "$S" > "$S/opener.sh"
    chmod +x "$S/opener.sh"
    MINE=$(cksum < "$SUPPORT/config.toml")$(cksum < "$SUPPORT/saves/save1.bin")
}
# runs the launcher: run ARCH [VARIABLE=VALUE...] -- ARGUMENTS
run() {
    arch=$1; shift
    vars=
    while [ "$1" != "--" ]; do vars="$vars $1"; shift; done
    shift
    # shellcheck disable=SC2086
    env HOME="$S/home" HALO_MAC_UPDATE_HEADLESS=1 HALO_MAC_UPDATE_OPENER="$S/opener.sh" $vars arch -$arch "$L" "$@" > "$S/out.txt" 2>&1
    RC=$?
}
user_data_untouched() { [ "$(cksum < "$SUPPORT/config.toml")$(cksum < "$SUPPORT/saves/save1.bin")" = "$MINE" ]; }
no_work_folders() { [ -z "$(ls -a "$S/apps" | grep 'PowerSpark-update-')" ]; }
unchanged() { [ "$(version_of "$APP")" = "0.1" ] && [ ! -e "$APP.old" ] && no_work_folders; }
branding() { echo "HALO_MAC_BRANDING=$FIX/branding-$1.plist"; }

for ARCH in $ARCHS; do
    echo "== $ARCH"

    new_case "uptodate-$ARCH"
    run $ARCH $(branding current) -- --check-updates
    has "PowerSpark is up to date" && lacks "available" && unchanged && ok "$ARCH: nothing newer says so" || bad "$ARCH: nothing newer says so"

    new_case "install-$ARCH"
    run $ARCH $(branding ok) HALO_UPDATE_ANSWER=yes -- --check-updates
    sleep 6
    if [ "$(version_of "$APP")" = "0.2" ] && [ -f "$APP/Contents/Resources/marker.txt" ] && [ ! -e "$APP.old" ] &&
        no_work_folders && grep -q "PowerSpark.app" "$S/opened.txt" 2> /dev/null && user_data_untouched; then
        ok "$ARCH: update installs, reopens the app, removes .old, leaves user data alone"
    else
        bad "$ARCH: update installs, reopens the app, removes .old, leaves user data alone"
        ls -la "$S/apps"
    fi
    run $ARCH $(branding ok) -- --dry-run
    has "updates:       https://localhost:18443/appcast-ok.plist" && ok "$ARCH: the installed app runs (version $(version_of "$APP"))" || bad "$ARCH: the installed app runs"

    new_case "tampered-$ARCH"
    run $ARCH $(branding tampered) HALO_UPDATE_ANSWER=yes -- --check-updates
    has "does not match its checksum" && unchanged && user_data_untouched && ok "$ARCH: a changed zip is refused" || bad "$ARCH: a changed zip is refused"

    new_case "badsig-$ARCH"
    run $ARCH $(branding badsig) HALO_UPDATE_ANSWER=yes -- --check-updates
    has "not signed with PowerSpark's release key" && unchanged && ok "$ARCH: a release signed by another key is refused" || bad "$ARCH: a release signed by another key is refused"

    new_case "wrongid-$ARCH"
    run $ARCH $(branding wrongid) HALO_UPDATE_ANSWER=yes -- --check-updates
    has "not PowerSpark" && unchanged && ok "$ARCH: a signed zip of another app is refused" || bad "$ARCH: a signed zip of another app is refused"

    new_case "truncated-$ARCH"
    run $ARCH $(branding truncated) HALO_UPDATE_ANSWER=yes -- --check-updates
    has "cut short" && unchanged && ok "$ARCH: a download that stops halfway is refused" || bad "$ARCH: a download that stops halfway is refused"

    new_case "chunked-$ARCH"
    run $ARCH $(branding chunked) HALO_UPDATE_ANSWER=yes -- --check-updates
    sleep 6
    [ "$(version_of "$APP")" = "0.2" ] && ok "$ARCH: a chunked feed and download install" || bad "$ARCH: a chunked feed and download install"

    new_case "redirect-$ARCH"
    run $ARCH $(branding redirect) HALO_UPDATE_ANSWER=yes -- --check-updates
    sleep 6
    [ "$(version_of "$APP")" = "0.2" ] && ok "$ARCH: redirects are followed" || bad "$ARCH: redirects are followed"

    new_case "later-$ARCH"
    run $ARCH $(branding ok) HALO_UPDATE_ANSWER=no -- --check-updates
    has "version 0.2 is available" && unchanged && ok "$ARCH: Later changes nothing" || bad "$ARCH: Later changes nothing"
done

# the rest need no second architecture
ARCH=$(echo "$ARCHS" | awk '{print $1}')

new_case readonly
chmod 555 "$S/apps"
run $ARCH $(branding ok) HALO_UPDATE_ANSWER=yes -- --check-updates
chmod 755 "$S/apps"
has "cannot replace itself" && has "Nothing was changed" && has "Your PowerSpark was not changed" && unchanged && ok "a folder that cannot be written gets a clear message" || bad "a folder that cannot be written gets a clear message"

new_case down
run $ARCH $(branding down) -- --check-updates
has "could not look for updates" && has "connect" && unchanged && ok "asked, an unreachable site is said" || bad "asked, an unreachable site is said"
rm -f "$SUPPORT/update-check"
run $ARCH $(branding down) -- --update-launch-check
lacks "could not look for updates" && has "could not look" && ok "at launch, an unreachable site is only logged" || bad "at launch, an unreachable site is only logged"

new_case wrongca
run $ARCH $(branding wrongca) -- --check-updates
has "handshake failed" && unchanged && ok "a certificate that the CA list does not vouch for is refused" || bad "a certificate that the CA list does not vouch for is refused"

new_case status
run $ARCH $(branding status) -- --check-updates
has "HTTP 503" && ok "a feed that answers 503 is said" || bad "a feed that answers 503 is said"

new_case never
run $ARCH $(branding ok) HALO_UPDATE_ANSWER=never -- --update-launch-check
grep -q "auto = false" "$SUPPORT/config.toml" && grep -q 'mode = "windowed"' "$SUPPORT/config.toml" && unchanged && ok "Never Ask Again writes update.auto = false and keeps the rest" || bad "Never Ask Again writes update.auto = false and keeps the rest"
rm -f "$SUPPORT/update-check"
run $ARCH $(branding ok) -- --update-launch-check
has "no update check today" && ok "and the launch no longer looks" || bad "and the launch no longer looks"
run $ARCH $(branding ok) HALO_UPDATE_AUTO=true -- --update-launch-check
has "version 0.2 is available" && ok "HALO_UPDATE_AUTO overrides the setting" || bad "HALO_UPDATE_AUTO overrides the setting"

new_case daily
run $ARCH $(branding ok) HALO_UPDATE_ANSWER=no -- --update-launch-check
has "version 0.2 is available" && [ -f "$SUPPORT/update-check" ] && ok "the launch looks the first time" || bad "the launch looks the first time"
run $ARCH $(branding ok) HALO_UPDATE_ANSWER=no -- --update-launch-check
has "no update check today" && ok "and not again the same day" || bad "and not again the same day"
echo 1 > "$SUPPORT/update-check"
run $ARCH $(branding ok) HALO_UPDATE_ANSWER=no -- --update-launch-check
has "version 0.2 is available" && ok "and again after a day" || bad "and again after a day"

new_case nofeed
run $ARCH $(branding nofeed) -- --check-updates
has "no update feed" && ok "a build with no feed says so when asked" || bad "a build with no feed says so when asked"
run $ARCH $(branding nofeed) -- --update-launch-check
has "no update check today" && ok "and does not look at launch" || bad "and does not look at launch"
run $ARCH $(branding nofeed) -- --dry-run
has "updates:       off" && ok "--dry-run says updates are off" || bad "--dry-run says updates are off"
run $ARCH $(branding ok) -- --dry-run
has "updates:       https://localhost:18443/appcast-ok.plist" && ok "--dry-run names the feed" || bad "--dry-run names the feed"

new_case layout
run $ARCH $(branding ok) -- --update-dump
[ $RC -eq 0 ] && has "0 do not fit" && lacks "DOES NOT FIT" && ok "the update windows' text fits (--update-dump)" || bad "the update windows' text fits (--update-dump)"

echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
