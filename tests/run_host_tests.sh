#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out_dir="${TMPDIR:-/tmp}/ds5dongle-host-tests"
mkdir -p "$out_dir"

g++ -std=c++20 -Wall -Wextra -Werror -pedantic \
    -I"$repo_dir/src" \
    "$repo_dir/src/dualsense_parser.cpp" \
    "$repo_dir/src/switch_pro_protocol.cpp" \
    "$repo_dir/src/switch_rumble.cpp" \
    "$repo_dir/tests/test_switch_pro.cpp" \
    -o "$out_dir/test_switch_pro"

"$out_dir/test_switch_pro"
