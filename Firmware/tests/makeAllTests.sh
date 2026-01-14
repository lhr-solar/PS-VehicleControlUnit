#!/bin/bash

# Define color codes
RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[0;33m'
BLUE=$'\033[0;34m'
NC=$'\033[0m' # No Color

script_dir=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
makefile_dir=$(cd -- "$script_dir/.." &> /dev/null && pwd)

echo -e "Makefile dir: ${makefile_dir}"

# Default values
VERBOSE=false
MAKE_FLAGS="-B -s"

# Usage function
usage() {
    echo -e "${YELLOW}Usage: $0 [-v]"
    echo "  -v    Enable verbose output (show commands as they're run)"
    echo "  -h    Show this help message and exit${NC}"
    exit 1
}

# Parse arguments
while getopts ":vh" opt; do
    case ${opt} in
        v )
            VERBOSE=true
            set -x
            MAKE_FLAGS="-B"
            ;;
        h )
            usage
            ;;
        \? )
            echo -e "${RED}[ERROR] Invalid option: -$OPTARG${NC}"
            usage
            ;;
    esac
done

export script_dir makefile_dir RED GREEN YELLOW BLUE NC

port_list=(stm32g473xx)
echo -e "${BLUE}----------------------------------------${NC}"

test_list=()
shopt -s nullglob
for test_file in "$script_dir"/*_test.c; do
    test_name=$(basename "$test_file" .c)
    test_name="${test_name%_test}"   # remove trailing "_test"
    test_list+=("$test_name")
done
shopt -u nullglob

# Check if test_list is empty
if [ ${#test_list[@]} -eq 0 ]; then
    echo -e "${RED}[ERROR] Something is horribly wrong. No test files found in the tests directory.${NC}"
    exit 1
fi

# Function to compile a single test for a port with tagged output
compile_test() {
    local port=$1
    local test_name=$2

    echo -e "${BLUE}[INFO] Compiling the test - $test_name for $port${NC}"

    output=$(make -C "$makefile_dir" TEST="$test_name")
    error_code=$?
    if [ $error_code -ne 0 ]; then
        printf "${RED}[%s:%s] %s${NC}\n" "$port" "$test_name" "$output"
        echo -e "${RED}[ERROR] Errors occurred while compiling $test_name.c using $port${NC}"
        return 1
    fi
    
    echo -e "${GREEN}[INFO] Successfully compiled $test_name.c : $port${NC}"
    if [ "$VERBOSE" = true ]; then
        echo -e "${GREEN}[INFO] Output: $output${NC}"
    fi

    return 0
}

# Export function to make it available to GNU Parallel
export -f compile_test

# Use GNU Parallel to run compilations in parallel
if ! command -v parallel &> /dev/null; then
    echo -e "${RED}[ERROR] GNU Parallel is not installed. Please install it to run this script.${NC}"
    echo -e "${YELLOW}[INFO] See: sudo apt install parallel${NC}"
    exit 1
fi

if ! parallel --halt now,fail=1 -j $(nproc) compile_test \
    ::: "${port_list[@]}" \
    ::: "${test_list[@]}"; then
    echo -e "${RED}[ERROR] Error: Some builds failed${NC}"
    exit 1
fi

echo -e "${GREEN}[INFO] Jolly Good! All tests compiled successfully${NC}"