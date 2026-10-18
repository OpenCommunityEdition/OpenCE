#!/bin/sh
# usage: sweep6.sh replay capture log
r=$1; c=$2; log=$3
: > "$log"
for opt in "-X 4000000" "-X 4000000 -A d" "-X 1000000" "-X 1000000 -A d" "-X 300000" "-X 300000 -A d" "-X 4000000 -A dt" "-X 4000000 -A dc"; do
  echo "### $opt" >> "$log"
  "$r" "$c" -q -n 100 -m PF -v $opt >> "$log" 2>&1
done
echo "### done" >> "$log"
