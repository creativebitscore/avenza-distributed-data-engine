#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"; mkdir -p "$root/data"
for i in 1 2 3; do port=$((7000+i)); "$root/build/avenza-server" --port "$port" --data-dir "$root/data/node$i" >"$root/data/node$i.log" 2>&1 & echo $!; done
