#!/bin/bash
set -euo pipefail
WP1980_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$WP1980_ROOT/tooling/env.sh"
make -C "$WP1980_ROOT/native" -f Makefile.wp1980 -j4 "$@"
