#!/usr/bin/env bash
set -euo pipefail
target="${1:?intro/entrance/presenter/frontend/royal/tag}"
: "${WF_REFERENCES_ROOT:?Set root created by restore_packages.py}"
: "${YAUL_INSTALL_ROOT:?Set the supplied SDK}"
export YAUL_ARCH_SH_PREFIX=sh2eb-elf YAUL_PROG_SH_PREFIX=sh2eb-elf YAUL_ARCH_M68K_PREFIX=m68keb-elf YAUL_OPTION_MALLOC_IMPL=tlsf
export PATH="$YAUL_INSTALL_ROOT/bin:$PATH"
export MAKE_ISO_XORRISOFS="${MAKE_ISO_XORRISOFS:-$(command -v xorrisofs)}"
export NOCOLOR=1 SILENT=1
case "$target" in
frontend)
 cd "$WF_REFERENCES_ROOT"/frontend/WF-SATURN-MODE-ROSTER-HARDWARE-PASS-HANDOFF-R01/REFERENCE-SOURCE
 make -j4 SH_BUILD_DIR=rebuild01 SH_OUTPUT_DIR=rebuild01-disc
 ;;
intro)
 cd "$WF_REFERENCES_ROOT"/intro/WF-WP0-R04/source
 make -j4 SH_BUILD_DIR=rebuild01 SH_OUTPUT_DIR=rebuild01-disc
 ;;
entrance)
 cd "$WF_REFERENCES_ROOT"/entrance/WF-WP01-R02-FUENTES/source
 make -j4 SH_BUILD_DIR=rebuild01 SH_OUTPUT_DIR=rebuild01-disc
 ;;
presenter)
 cd "$WF_REFERENCES_ROOT"/presenter/WP02-Saturn-TAG-R02/source/scene
 WP02_MKISOFS=/usr/bin/xorrisofs python3 build.py
 ;;
royal)
 cd "$WF_REFERENCES_ROOT"/royal/wp1980/work/hwfix01/native
 make -f Makefile.wp1980 -j4 WP1980_AUDIO=0 WP1980_CART=0 SH_BUILD_DIR=rebuild01-base SH_OUTPUT_DIR=rebuild01-base-disc
 python3 "$WF_REFERENCES_ROOT"/royal/wp1980/validation/verify-hwram-budget.py rebuild01-base/wrestlefest-wp1980.map
 python3 "$WF_REFERENCES_ROOT"/royal/wp1980/validation/verify_saturn_stack_layout.py rebuild01-base/wrestlefest-wp1980.elf
 make -f Makefile.wp1980 -j4 WP1980_AUDIO=0 WP1980_CART=1 SH_BUILD_DIR=rebuild01-cart SH_OUTPUT_DIR=rebuild01-cart-disc
 python3 "$WF_REFERENCES_ROOT"/royal/wp1980/validation/verify-hwram-budget.py rebuild01-cart/wrestlefest-wp1980.map
 python3 "$WF_REFERENCES_ROOT"/royal/wp1980/validation/verify_saturn_stack_layout.py rebuild01-cart/wrestlefest-wp1980.elf
 ;;
tag)
 cd "$WF_REFERENCES_ROOT"/tag_r3/wp2015/work/native/src/runtime
 make -f Makefile.wp2015 -j4 WP2015_AUDIO=0 SH_BUILD_DIR=rebuild01 SH_OUTPUT_DIR=rebuild01-disc
 ;;
*) exit 2 ;;
esac
