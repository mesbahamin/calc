#!/usr/bin/env bash

set -e                     # fail if any command has a non-zero exit status
set -u                     # fail if any undefined variable is referenced
set -o pipefail            # propagate failure exit status through a pipeline
shopt -s globstar nullglob # enable recursive and null globbing

exe_name="calc"
build_dir="."

cc=clang
source_files=("calc.c")
cflags=("-std=c99" "-Wall" "-Wextra" "-Wshadow" "-Wsign-compare" "-Wno-missing-braces")
debug_flags=("-g" "-Og" "-Werror")
# shellcheck disable=SC2207
ldflags=()

build_debug() {
    debug_dir="${build_dir}"
    debug_path="${debug_dir}/${exe_name}"
    mkdir -p "$debug_dir"
    (
        set -x;
        "$cc" "${cflags[@]}" "${debug_flags[@]}" "${source_files[@]}" -o "$debug_path" "${ldflags[@]}"
    )
}

usage() {
    echo "build.sh - Build ${exe_name}"
    echo " "
    echo "build.sh [options]"
    echo " "
    echo "options:"
    echo "-h, --help                show help"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        *)
            break
            ;;
    esac
done

build_debug

exit 0
