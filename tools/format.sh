#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

CLANG_FORMAT=${CLANG_FORMAT:-clang-format-18}

find src tests tools/lmc-agent -type f \( -name '*.cpp' -o -name '*.h' \) | sort | xargs "$CLANG_FORMAT" -i
echo "Formatted src, tests and tools/lmc-agent with $CLANG_FORMAT"
