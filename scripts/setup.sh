#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cmake -S "$root" -B "$root/build" -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-Release}" -DAVENZA_FETCH_GTEST="${FETCH_GTEST:-OFF}"
cmake --build "$root/build" --parallel 2
ctest --test-dir "$root/build" --output-on-failure
