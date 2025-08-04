#!/bin/bash

SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
TARGET_DIR="${SCRIPT_DIR}/Google_comp/844/WFR_F2"
MAX_JOBS=4 

echo "SCRIPT_DIR=${SCRIPT_DIR}"
echo "TARGET_DIR=${TARGET_DIR}"
echo "Running all .sh scripts in ${TARGET_DIR} (dynamic parallelism, max ${MAX_JOBS} jobs)"

# Collect all scripts
mapfile -t all_scripts < <(find "${TARGET_DIR}" -type f -name "*.sh" | sort)

# Function to count running background jobs
num_jobs() {
    jobs -rp | wc -l
}

for script in "${all_scripts[@]}"; do
    chmod +x "$script"
    echo "Launching: $script"
    VMEM_LIMIT=$((36 * 1024 * 1024))
    bash -c "ulimit -v \$VMEM_LIMIT; exec \"$script\"" &

    # Wait if too many jobs running
    while [ "$(num_jobs)" -ge "$MAX_JOBS" ]; do
        sleep 1
    done
done

# Wait for remaining jobs to finish
wait
echo "All Google_comp_844 scripts completed."