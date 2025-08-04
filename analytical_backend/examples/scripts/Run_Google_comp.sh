#!/bin/bash
# Get current directory
SCRIPT_DIR=$(dirname "$(realpath "$0")")

# Tailor Google_comp directory
TARGET_DIR="${SCRIPT_DIR}/Google_comp"

echo "Running all .sh scripts in ${TARGET_DIR}"

# Traverse and run all the .sh scripts in Google_comp directory
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

echo "All Google_comp scripts completed."