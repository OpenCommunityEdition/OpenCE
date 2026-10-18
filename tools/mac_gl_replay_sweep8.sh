#!/bin/sh
# usage: sweep8.sh replay capture log
r=$1; c=$2; log=$3
: > "$log"
for opt in "-Z" "-Z -A d" "-Z -A s"; do
  echo "### $opt" >> "$log"
  "$r" "$c" -q -n 100 -m PF -v $opt >> "$log" 2>&1
done
echo "### done" >> "$log"
