#!/bin/bash

silk_root=$(dirname $(realpath -- "$0"))
silk_starting_dir=$(dirname $PWD)
silk_gcc=1
silk_compiler="${CC:-gcc}"
silk_run=1
silk_file="$(realpath -- silk.c)"  # Source file name to compile
silk_output="./silk.bin"           # Executable name
silk_include_dir="$silk_root/"     # Include directory to locate the silk.h file

while (( "$#" )); do
    if [ "$1" == "clang" ];  then silk_clang=1; silk_compiler="${CC:-clang}"; unset silk_gcc; fi
    if [ "$1" == "gcc" ];    then silk_gcc=1;   silk_compiler="${CC:-gcc}";   unset silk_clang; fi
    if [ "$1" == "help" ];   then silk_help=1; fi
    if [ "$1" == "run" ];    then silk_run=1; fi
    if [ "$1" == "--pedantic" ]; then silk_pedantic=1; fi
    if [ "$1" == "--file" ]; then silk_file=$2; shift; fi
    if [ "$1" == "--output" ]; then silk_output=$2; shift; fi 
    if [ "$1" == "--include-dir" ]; then silk_include_dir=$2; shift; fi
    shift
done

# Extract parent directory of the silk.c file path
silk_dir=${silk_file%/*}
# Extract filename of the file path
silk_filename=$(basename "$silk_file")

cd "$silk_dir"

cleanup() {
  cd "$silk_starting_dir"
}

# Restore initial directory on exit
trap cleanup EXIT

# Remove previous executable if it exists.
if [ -f "$silk_output" ]; then
   rm "$silk_output"
fi

if [ -v silk_gcc ] && [ -v silk_pedantic ]; then silk_cxflags="-std=c89 -pedantic -Werror -Wextra" ; fi

# Check if there is a value in silk_gcc.
if [ -v silk_gcc ]; then
   $silk_compiler $silk_cxflags -g -I $silk_include_dir -o "$silk_output" -O0 $silk_filename || { echo "'$silk_compiler' exited with $?"; exit 1; }
fi

# Check if there is a value in silk_run.
if [ -v silk_run ]; then
   "$silk_output" || { echo "'$silk_output' exited with $?"; exit 1; }
fi
