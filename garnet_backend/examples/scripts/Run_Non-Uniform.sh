#!/bin/bash
SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
TARGET_DIR="${SCRIPT_DIR}/Non-Uniform"

MAX_JOBS=$(nproc)  
current_jobs=0

echo "SCRIPT_DIR=${SCRIPT_DIR}"
echo "TARGET_DIR=${TARGET_DIR}"

echo "Running all .sh scripts recursively in ${TARGET_DIR} (basic parallel mode)"

for script in $(find "${TARGET_DIR}" -type f -name "*.sh" | sort); do
    chmod +x "$script"
    echo "Executing: $script"
    "$script" &

    ((current_jobs++))
    if ((current_jobs >= MAX_JOBS)); then
        wait
        current_jobs=0
    fi
done

wait
echo "All Non-Uniform scripts completed in parallel."