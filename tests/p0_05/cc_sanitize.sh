#!/bin/sh
# Host-only sanitizer wrapper. Linker-only -no-pie must not reach Clang -c.
for arg do
    case "$arg" in
        -c|-S|-E)
            exec "${WF_HOST_CC:-gcc}" -fsanitize=address,undefined \
                -fno-omit-frame-pointer -fno-pie "$@"
            ;;
    esac
done
exec "${WF_HOST_CC:-gcc}" -fsanitize=address,undefined \
    -fno-omit-frame-pointer -fno-pie -no-pie "$@"
