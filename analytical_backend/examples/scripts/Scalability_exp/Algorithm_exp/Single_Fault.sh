#! /bin/bash
SCRIPT_DIR=$(dirname "$(realpath $0)")

# Relative paths to directories and binaries
BINARY=${SCRIPT_DIR}/../../../../build/astra_analytical/build/AnalyticalAstra/bin/AnalyticalAstra
NETWORK_DIR=${SCRIPT_DIR}/../../../../inputs/network/analytical/Scalability_exp/Algorithm
SYSTEM_DIR=${SCRIPT_DIR}/../../../../inputs/system/Synthetic_exp/Tolerance_single_NDTorus
WORKLOAD_DIR=${SCRIPT_DIR}/../../../../inputs/workload/Scalability_exp/Weak
STATS_DIR=${SCRIPT_DIR}/../../../../examples/results/Scalability_exp/Algorithm_weak/Single_Fault

# Ensure the STATS_DIR exists
mkdir -p "${STATS_DIR}"

# Calculate total experiments for the whole script run
total_experiments=0
system_file="${SYSTEM_DIR}/2D.txt"
if [[ -f "$system_file" ]]; then
    total_experiments=$(($total_experiments + 9))  # Each 2D system configuration matches 9 network files
fi
echo "There are total ${total_experiments} experiments."

# Initialize experiment index before starting all experiments
exp_index=0  

# Loop through workloads
for workload_file in ${WORKLOAD_DIR}/*.txt; do
    workload=$(basename "${workload_file}")

    # Process only 2D system file
    if [[ -f "${system_file}" ]]; then
        system=$(basename "${system_file}")
        dim="${system%.*}"  # Extract dimension part (2D)

        # Loop through all matching network files
        for network_file in ${NETWORK_DIR}/Fault_Tolerance_${dim}_*.json; do
            if [[ -f "${network_file}" ]]; then
                network=$(basename "${network_file}")

                echo "Running experiment: Workload=${workload}, System=${system}, Network=${network}"
                "${BINARY}" \
                  --run-name="Experiment_${workload%.txt}_${system%.*}_${network%.json}" \
                  --network-configuration="${network_file}" \
                  --system-configuration="${system_file}" \
                  --workload-configuration="${workload_file}" \
                  --path="${STATS_DIR}/" \
                  --num-passes=1 \
                  --total-stat-rows=$total_experiments \
                  --stat-row=$exp_index

                echo "Experiment with Workload=${workload}, System=${system}, Network=${network} completed."
                ((exp_index++))
            else
                echo "No matching network file for ${system_file}"
            fi
        done 
    fi
done

echo "All experiments completed."
