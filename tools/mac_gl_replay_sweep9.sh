#!/bin/sh
# usage: sweep9.sh replay capture log   -- text atlas size experiment
r=$1; c=$2; log=$3
: > "$log"
for e in 1024 512 256 4; do
  for a in "" d; do
    echo "### GLR_ATLAS_EDGE=$e -Z -A '$a'" >> "$log"
    GLR_ATLAS_EDGE=$e "$r" "$c" -q -n 100 -m PF -Z -A "$a" >> "$log" 2>&1
  done
done
echo "### done" >> "$log"
