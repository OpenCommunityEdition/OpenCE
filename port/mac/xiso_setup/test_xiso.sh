#!/bin/sh
# Runs xiso_cli (and the app's --test mode) over the synthetic images from
# make_xiso.py. Usage: test_xiso.sh IMAGES_DIR WORK_DIR [runner]
# (runner: e.g. "arch -ppc" to run the ppc half under Rosetta)
set -u
images=$1; work=$2; runner=${3:-}
here=$(cd "$(dirname "$0")" && pwd)
cli="$here/xiso_cli"
app="$here/OpenCE Setup.app/Contents/MacOS/OpenCE Setup"
pass=0; fail=0
check() { # name ok|refuse|either exit
    cname=$1; verdict=$2; rc=$3
    case "$verdict:$rc" in
        ok:0|refuse:1|either:0|either:1) pass=$((pass+1));;
        *) echo "FAIL $cname: expected $verdict, exit $rc"; fail=$((fail+1));;
    esac
}
for tool in cli app; do
rm -rf "$work"; mkdir -p "$work"
while read name verdict; do
    d="$work/$tool-$name"
    if [ $tool = cli ]; then $runner "$cli" install "$images/$name.xiso" "$d" >/dev/null 2>"$d.err"; rc=$?
    else $runner "$app" --test --image "$images/$name.xiso" --data "$d" >/dev/null 2>"$d.err"; rc=$?; fi
    check "$tool/$name" "$verdict" $rc
    # nothing but a whole maps folder may be left, and nothing may escape
    if [ -e "$d/.xiso-tmp" ] || [ -e "$d/maps.partial" ]; then echo "FAIL $tool/$name: temporary folder left"; fail=$((fail+1)); fi
    if [ $rc != 0 ] && [ -e "$d/maps" ]; then echo "FAIL $tool/$name: refused but maps exists"; fail=$((fail+1)); fi
    if [ -e "$d/escape.map" ] || [ -e "$d/ui.map" ] || [ -e "$work/escape.map" ] || [ -e "$work/ui.map" ]; then echo "FAIL $tool/$name: file escaped the maps folder"; fail=$((fail+1)); fi
    if [ $rc = 0 ] && [ $name != big ] && [ $name != dirloop ] && [ $name != selfloop ] && [ $name != name_overrun ]; then
        if ! diff -r "$images/ref/maps" "$d/maps" >/dev/null 2>&1; then
            # evilnames: the plain files must all be there, with no extras
            for f in "$images"/ref/maps/*; do cmp -s "$f" "$d/maps/$(basename "$f")" || { echo "FAIL $tool/$name: $(basename "$f") differs"; fail=$((fail+1)); }; done
            n=$(ls "$d/maps" | wc -l | tr -d ' '); [ "$n" = 9 ] || { echo "FAIL $tool/$name: $n files in maps"; fail=$((fail+1)); }
        fi
    fi
    [ -s "$d.err" ] && echo "  $tool/$name -> $(tail -1 "$d.err")"
done < "$images/expect.txt"
done
# an existing maps folder is not overwritten, and verify works on the result
d="$work/again"
$runner "$cli" install "$images/good.xiso" "$d" >/dev/null 2>&1; check "first" ok $?
$runner "$cli" install "$images/good.xiso" "$d" >/dev/null 2>&1; check "again" refuse $?
$runner "$cli" verify "$d/maps" >/dev/null 2>&1; check "verify" ok $?
# a stale temporary folder from an interrupted run is cleared
d="$work/stale"; mkdir -p "$d/.xiso-tmp/maps.partial"; echo x > "$d/.xiso-tmp/maps.partial/junk"
$runner "$cli" install "$images/good.xiso" "$d" >/dev/null 2>&1; check "stale" ok $?
[ -e "$d/.xiso-tmp" ] && { echo "FAIL stale: temporary left"; fail=$((fail+1)); }
# cancelling part way (exit 2), leaving nothing
d="$work/cancel"
XISO_CLI_CANCEL_AFTER_MB=5 $runner "$cli" install "$images/big.xiso" "$d" >/dev/null 2>&1; rc=$?
[ $rc = 2 ] && pass=$((pass+1)) || { echo "FAIL cancel: exit $rc"; fail=$((fail+1)); }
ls -A "$d" | grep -q . && { echo "FAIL cancel: left $(ls -A "$d")"; fail=$((fail+1)); }
# the maps folder missing its a10.map is refused by verify
rm -f "$work/again/maps/a10.map"; $runner "$cli" verify "$work/again/maps" >/dev/null 2>&1; check "verify-missing" refuse $?
echo "passed $pass, failed $fail"
[ $fail = 0 ]
