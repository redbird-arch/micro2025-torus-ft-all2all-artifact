#!/bin/bash
# Get current directory
SCRIPT_DIR=$(dirname "$(realpath "$0")")

# Tailor Non-Uniform directory
TARGET_DIR="${SCRIPT_DIR}/Non-Uniform"

echo "Running all .sh scripts in ${TARGET_DIR}"

# Traverse and run all the .sh scripts in Non-Uniform directory
for script in "${TARGET_DIR}"/*.sh; do
    if [[ -x "$script" ]]; then
        echo "Executing: $(basename "$script")"
        "$script"
    else
        echo "Making executable and running: $(basename "$script")"
        chmod +x "$script"
        "$script"
    fi
done

echo "All Non-Uniform scripts completed."