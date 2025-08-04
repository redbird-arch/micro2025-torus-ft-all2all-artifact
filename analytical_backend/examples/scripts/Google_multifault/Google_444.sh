#! /bin/bash 
SCRIPT_DIR=$(dirname "$(realpath $0)")

# Relative paths to useful directories
BINARY=${SCRIPT_DIR}/../../../build/astra_analytical/build/AnalyticalAstra/bin/AnalyticalAstra
NETWORK_DIR=${SCRIPT_DIR}/../../../inputs/network/analytical/Google_comp
SYSTEM_DIR=${SCRIPT_DIR}/../../../inputs/system/Google_comp
WORKLOAD_DIR=${SCRIPT_DIR}/../../../inputs/workload
STATS_DIR=${SCRIPT_DIR}/../../../examples/results/Google_MultiFault

# Ensure the STATS_DIR exists
mkdir -p "${STATS_DIR}"

# Define network to run (only single fault)
NETWORK="TPUv4_4x4x4_SingleFault.json"

# Define workloads
WORKLOADS=("AllToAll_Synthetic_32KB.txt" "AllToAll_Synthetic_64KB.txt" "AllToAll_Synthetic_128KB.txt" "AllToAll_Synthetic_256KB.txt" "AllToAll_Synthetic_512KB.txt" "AllToAll_Synthetic_1MB.txt" "AllToAll_Synthetic_2MB.txt" "AllToAll_Synthetic_4MB.txt" "AllToAll_Synthetic_8MB.txt" "AllToAll_Synthetic_16MB.txt")

# Loop through Fault_2_* system configurations
for SYSTEM_CONFIG in "${SYSTEM_DIR}"/Fault_2_*.txt; do
    SYSTEM_NAME=$(basename "${SYSTEM_CONFIG}" .txt)

    # Set statistics path for this system config
    STATS_PATH_SYSTEM="${STATS_DIR}/${SYSTEM_NAME}"
    mkdir -p "$STATS_PATH_SYSTEM"

    total_experiments_for_system=${#WORKLOADS[@]}
    exp_index=0

    # Loop through workloads
    for workload in "${WORKLOADS[@]}"; do
        WORKLOAD_PATH="${WORKLOAD_DIR}/${workload}"
        run_name="${workload%.txt}_${SYSTEM_NAME}_exp${exp_index}"

        echo "Running experiment: $run_name"
        "${BINARY}" \
          --run-name="${run_name}" \
          --network-configuration="${NETWORK_DIR}/${NETWORK}" \
          --system-configuration="${SYSTEM_CONFIG}" \
          --workload-configuration="${WORKLOAD_PATH}" \
          --path="${STATS_PATH_SYSTEM}/" \
          --num-passes=1 \
          --total-stat-rows=$total_experiments_for_system \
          --stat-row=$exp_index

        ((exp_index++))
        echo "Experiment ${run_name} completed."
    done
    echo "All experiments for ${SYSTEM_NAME} completed."
done

echo "All experiments completed."
