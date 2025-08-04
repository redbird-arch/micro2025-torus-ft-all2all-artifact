#!/bin/bash
# Get current directory
SCRIPT_DIR=$(dirname "$(realpath "$0")")

# Tailor Real_Model directory
TARGET_DIR="${SCRIPT_DIR}/Real_Model"

echo "Running all .sh scripts in ${TARGET_DIR}"

# Traverse and run all the .sh scripts in Real_Model directory
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

echo "All Real_Model scripts completed."