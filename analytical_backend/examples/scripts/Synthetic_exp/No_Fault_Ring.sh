#! /bin/bash
SCRIPT_DIR=$(dirname "$(realpath $0)")

# Relative paths to directories and binaries
WORKLOAD_DIR="${SCRIPT_DIR}/../../../inputs/workload/Synthetic_exp"
SYSTEM_DIRS=("${SCRIPT_DIR}/../../../inputs/system/Synthetic_exp/Ring_No_Fault_Pipeline" "${SCRIPT_DIR}/../../../inputs/system/Synthetic_exp/Ring_No_Fault_NDTorus")
NETWORK_DIR="${SCRIPT_DIR}/../../../inputs/network/analytical/Synthetic_exp"
STATS_DIR="${SCRIPT_DIR}/../../../examples/results/Synthetic_exp/No_Fault_Ring"
BINARY="${SCRIPT_DIR}/../../../build/astra_analytical/build/AnalyticalAstra/bin/AnalyticalAstra"

# Ensure the STATS_DIR exists
mkdir -p "${STATS_DIR}"

# Calculate total experiments for the whole script run
total_experiments=0
for system_dir in "${SYSTEM_DIRS[@]}"; do
    for system_file in ${system_dir}/*.txt; do
        if [[ -f "$system_file" ]]; then
            ((total_experiments+=10))  # Each system configuration matches 10 workloads
        fi
    done
done
echo "There are total ${total_experiments} experiments."


# Initialize experiment index before starting all experiments
exp_index=0  

# Loop through workloads
for workload_file in ${WORKLOAD_DIR}/*.txt; do
    workload=$(basename "${workload_file}")

    # Loop through system directories
    for system_dir in "${SYSTEM_DIRS[@]}"; do
        # Extract system type (Themis or NDTorus) from directory name
        system_type=$(basename "${system_dir}" | grep -oE "(Themis|NDTorus)")

        # Loop through system files in each directory
        for system_file in ${system_dir}/*.txt; do
            system=$(basename "${system_file}")
            dim="${system%.*}"  # Extract dimension part (e.g., 2D, 3D, 4D)

            # Find matching network configuration
            network_file="${NETWORK_DIR}/No_Fault_${dim}.json"
            if [[ -f "${network_file}" ]]; then
                network=$(basename "${network_file}")

                echo "Running experiment: Workload=${workload}, System=${system_type}_${system}, Network=${network}"
                "${BINARY}" \
                  --run-name="Experiment_${workload%.txt}_${system_type}_${system%.*}_${network%.json}" \
                  --network-configuration="${network_file}" \
                  --system-configuration="${system_file}" \
                  --workload-configuration="${workload_file}" \
                  --path="${STATS_DIR}/" \
                  --num-passes=1 \
                  --total-stat-rows=$total_experiments \
                  --stat-row=$exp_index

                echo "Experiment with Workload=${workload}, System=${system_type}_${system}, Network=${network} completed."
                ((exp_index++))
            else
                echo "No matching network file for ${system_file}"
            fi
        done
    done
done

echo "All experiments completed."
