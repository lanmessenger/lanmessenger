#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

CLANG_FORMAT=${CLANG_FORMAT:-clang-format-18}

find lmc/src -type f \( -name '*.cpp' -o -name '*.h' \) | sort | xargs "$CLANG_FORMAT" -i
echo "Formatted lmc/src with $CLANG_FORMAT"
