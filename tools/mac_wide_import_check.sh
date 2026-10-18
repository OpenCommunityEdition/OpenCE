#!/bin/sh
# Reject Darwin wchar32 imports from a manifest of -fshort-wchar engine objects.
# A manifest has one object path per line, as emitted by native build checks.
set -eu
if [ "$#" -ne 1 ]; then
    echo "usage: $0 engine-object-manifest" >&2
    exit 2
fi
status=0
while IFS= read -r object || [ -n "$object" ]; do
    [ -n "$object" ] || continue
    if [ ! -f "$object" ]; then
        echo "missing engine object: $object" >&2
        status=1
        continue
    fi
    imports=$(nm -u "$object") || exit 2
    bad=$(printf '%s\n' "$imports" | awk '$1 ~ /^_(wcs[a-z]|wmem[a-z]|[vf]*[sn]*wprintf|[vf]*[sn]*wscanf|fgetwc|fgetws|fputwc|fputws|getwchar|putwchar|ungetwc|tow[a-z]|isw[a-z]|mbtowc|mbstowcs|mbrtowc|mbsrtowcs|wctomb|wcrtomb)/ {print $1}')
    if [ -n "$bad" ]; then
        printf '%s: incompatible Darwin wide-character imports\n%s\n' "$object" "$bad" >&2
        status=1
    fi
done < "$1"
exit "$status"
