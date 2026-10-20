#!/bin/sh
appname=lan-messenger
script="$0"
n=0
while [ -L "$script" ]; do
  n=$((n+1))
  if [ "$n" -gt 20 ]; then
    echo 'too many symlinks' >&2
    exit 1
  fi
  link="$(readlink "$script")"
  case "$link" in
    /*) script="$link" ;;
    *) script="$(dirname "$script")/$link" ;;
  esac
done
appdir="$(cd "$(dirname "$script")" && pwd)" || exit 1
LD_LIBRARY_PATH="$appdir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH
exec "$appdir/$appname" "$@"
