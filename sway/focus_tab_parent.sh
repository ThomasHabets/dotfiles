#!/bin/bash
#
# If the current window is in a tabbed group, focus to the tabbing container and
# then run any commands provided as args to the script.
#
set -e
BASE="$(dirname "$0")"

DEPTH_ORIG=$(swaymsg -t get_tree | jq -f "$BASE/focus_path.jq")
DEPTH=$DEPTH_ORIG
COMMANDS=()
if [[ "$DEPTH" != "null" ]]; then
  while true; do
    COMMANDS+=("focus parent")
    if [[ "$DEPTH" == "0" ]]; then
      break
    fi
    DEPTH=$((DEPTH-1))
    if [[ ${#COMMANDS[@]} -gt 10 ]]; then
      echo "focus_parent_tab: too much depth: $DEPTH_ORIG" >&2
      break
    fi
  done
fi  
while [[ ! "$1" = "" ]]; do
  if [[ ${#COMMANDS[@]} -gt 10 ]]; then
    echo "focus_parent_tab: too much depth: $DEPTH_ORIG" >&2
    break
  fi
  COMMANDS+=("$1")
  shift
done
  
S="$(IFS=,; echo "${COMMANDS[*]}")"
exec swaymsg "$S"
