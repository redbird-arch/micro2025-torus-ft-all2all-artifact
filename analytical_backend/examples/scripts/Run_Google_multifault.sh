#!/bin/bash
# Get current directory
SCRIPT_DIR=$(dirname "$(realpath "$0")")

# Tailor Google_multifault directory
TARGET_DIR="${SCRIPT_DIR}/Google_multifault"

echo "Running all .sh scripts in ${TARGET_DIR}"

# Traverse and run all the .sh scripts in Google_multifault directory
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

echo "All Google_multifault scripts completed."