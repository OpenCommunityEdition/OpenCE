#!/bin/sh
# usage: sweep10.sh replay capture log   -- the single ablations again with the text atlas made 4x4 (-Z)
r=$1; c=$2; log=$3
: > "$log"
echo "### -Z -A '' -m PFT" >> "$log"
"$r" "$c" -n 100 -m PFT -Z >> "$log" 2>&1
for a in a b c e f F g G h x s S; do
  echo "### -Z -A '$a'" >> "$log"
  "$r" "$c" -q -n 100 -m PF -Z -A "$a" >> "$log" 2>&1
done
echo "### done" >> "$log"
