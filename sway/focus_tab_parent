#!/bin/bash
#
# If the current window is in a tabbed group, focus to the tabbing container and
# then run any commands provided as args to the script.
#
set -e
BASE="$(dirname "$0")"

DEPTH=$(swaymsg -t get_tree | jq -f "$BASE/focus_path.jq")
COMMANDS=()
if [[ "$DEPTH" != "null" ]]; then
  while true; do
    COMMANDS+=("focus parent")
    if [[ "$DEPTH" == "0" ]]; then
      break
    fi
    DEPTH=$((DEPTH-1))
  done
fi  
while [[ ! "$1" = "" ]]; do
  COMMANDS+=("$1")
  shift
done
  
S="$(IFS=,; echo "${COMMANDS[*]}")"
exec swaymsg "$S"
