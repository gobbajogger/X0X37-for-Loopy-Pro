#!/bin/sh
set -eu
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo 'usage: sh run.sh SYNTHONE_SOURCE_ROOT [smoke|o2|os|sanitize]' >&2
  exit 2
fi
case "$1" in /*) source_input=$1 ;; *) source_input=$(pwd)/$1 ;; esac
variant=${2:-o2}
project_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
exec python3 -I "$project_dir/mutation_checks.py" --build "$source_input" "$variant"
