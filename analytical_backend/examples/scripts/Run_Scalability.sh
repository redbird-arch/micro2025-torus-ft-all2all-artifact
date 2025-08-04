#!/bin/bash
# Get current directory
SCRIPT_DIR=$(dirname "$(realpath "$0")")

# Tailor Scalability_exp directory
TARGET_DIR="${SCRIPT_DIR}/Scalability_exp"

echo "Running all .sh scripts in subdirectories of ${TARGET_DIR}"

# Traverse and run all the .sh scripts recursively in subdirectories
find "${TARGET_DIR}" -type f -name "*.sh" | sort | while read -r script; do
    if [[ -x "$script" ]]; then
        echo "Executing: $(basename "$script")"
        "$script"
    else
        echo "Making executable and running: $(basename "$script")"
        chmod +x "$script"
        "$script"
    fi
done

echo "All Scalability_exp scripts completed."