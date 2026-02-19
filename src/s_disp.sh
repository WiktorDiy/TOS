#!/bin/sh

file="$1"
size=$(stat -c%s "$file")

# Print: FILENAME: SIZE   (size in green)
printf "%s: \033[32m%s\033[0m\n" "$file" "$size"
