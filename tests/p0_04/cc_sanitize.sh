#!/bin/sh
# Host-only compiler wrapper. Do not use as a Saturn SH-2 toolchain.
exec "${WF_HOST_CC:-gcc}" -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie "$@"
