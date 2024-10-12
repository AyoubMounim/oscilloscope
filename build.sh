#!/bin/bash

PROGNAME=${0##*/}
VERSION="0.1"
LIBS=()  # Paths to external libraries.

# Global variables here.
BUILD_DIR="build"

function set_up(){
    [[ -d "$BUILD_DIR" ]] || mkdir -p "$BUILD_DIR"
    return 0
}

function clean_up(){
    # Graceful exit clean up.
    return 0
}

function graceful_exit(){
    local error_code="$1"
    clean_up
    exit "${error_code:-0}"
}

function error_exit(){
    local error_msg="$1"
    printf "%s: %s\n" "$PROGNAME" "${error_msg:-"Unknown Error"}" >&2
    graceful_exit 1
}

function signal_exit(){
    local signal="$1"
    case "$signal" in
        INT)
            error_exit "Program interrupted by user."
            ;;
        TERM)
            error_exit "Program terminated."
            ;;
        *)
            error_exit "Program killed by unknown signal."
            ;;
    esac
}

function load_libs(){
    local i
    for i in $LIBS; do
        [[ -r "$i" ]] || error_exit "Library '$i' not found."
        source "$i" || error_exit "Failed to source library '$i'."
    done
    return 0
}

function usage(){
    printf "%s\n" "Usage: $PROGNAME [-h | --help]"
    printf "       %s\n" "$PROGNAME [options] args"
    return 0
}

function print_help(){
    cat <<- EOF
$PROGNAME ver. $VERSION
"program description"

$(usage)

Otions:
-h, --help          Display this help message.

EOF
    return 0
}

trap "signal_exit TERM" TERM HUP
trap "signal_exit INT" INT

load_libs

while [[ -n "$1" ]]; do
    case "$1" in
        -h | --help)
            print_help
            graceful_exit
            ;;
        -b | --build_dir)
            shift
            BUILD_DIR="$1"
            ;;
        -* | --*)
            usage >&2
            error_exit "Unknown option $1"
            ;;
        *)
            # Process argument
            ;;
    esac
    shift
done

set_up

gcc -ggdb -I./buffer/include main.c ./buffer/src/io_buffer.c -lraylib -lm -o $BUILD_DIR/oscilloscope
go build -o $BUILD_DIR/signal-generator signal_generator/signal_generator.go

graceful_exit

