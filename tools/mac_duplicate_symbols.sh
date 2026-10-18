#!/bin/sh
# MAC_DUPLICATE_SYMBOLS.SH <objects-list> [label]
#
# A symbol defined by two of the objects a link is given, before ld is the
# one to say so.
#
# The Mac session link is a list of objects: the ones compiled from this
# checkout, and the ones taken from a manifest built earlier. When a unit
# stops being compiled, or starts, nothing checks that the two sets still
# define each name once. Three builds in a row stopped on this while
# internet play went in: the manifest's offline P2P stub against the real
# p2p.o (p2p_hardware_id), engine_upstream_stubs.c against p2p.o again,
# and the tick build's mbedtls against the session's own
# (mbedtls_platform_zeroize). Each cost a full build to find.
#
# This reads the same object list the link is given and says which name is
# defined twice and by whom, in seconds and without linking.
#
# Not every repeated name is a fault: a tentative definition (nm's "C", an
# uninitialised file-scope variable) may appear in many objects and the
# linker merges them, so those are left alone. Only real definitions are
# counted.

set -u
list=${1:-}
label=${2:-}
if [ -z "$list" ] || [ ! -f "$list" ]; then
    echo "usage: mac_duplicate_symbols.sh <objects-list> [label]" >&2
    exit 2
fi

work=${TMPDIR:-/tmp}/mac-duplicate-symbols.$$
trap 'rm -f "$work" "$work.pairs"' EXIT HUP INT TERM

# (in batches: a session's list is over a thousand objects, more than one
# command line holds)
xargs nm -g < "$list" > "$work" 2>/dev/null || true

# nm prints a blank line, "<object>:", then its symbols; an undefined one
# has no address, so the type is the first field rather than the second
awk '
    /^[^ \t].*:$/ { file = substr($0, 1, length($0) - 1); next }
    NF == 3 { type = $2; name = $3 }
    NF == 2 { type = $1; name = $2 }
    NF < 2  { next }
    # U is a reference, not a definition; C is a tentative definition, which
    # the linker is allowed to merge
    type == "U" || type == "C" { next }
    # lower case is a local symbol, which cannot collide across objects
    type ~ /^[a-z]$/ { next }
    { print name "\t" file }
' "$work" | sort -u > "$work.pairs"

count=$(cut -f1 "$work.pairs" | uniq -d | wc -l | tr -d ' ')
if [ "$count" = 0 ]; then
    echo "${label:+$label: }no symbol is defined by two objects"
    exit 0
fi

echo "${label:+$label: }$count symbol(s) defined by more than one object:" >&2
for name in $(cut -f1 "$work.pairs" | uniq -d); do
    echo "  $name" >&2
    grep "^$name	" "$work.pairs" | cut -f2 | sed 's/^/    /' >&2
done
echo "" >&2
echo "One of them is meant to provide it. A unit that is no longer compiled" >&2
echo "must still be skipped where the manifest's objects are chosen" >&2
echo "(tools/mac_rendered_session_checks.sh), or its old object is linked" >&2
echo "beside the new one." >&2
exit 1
