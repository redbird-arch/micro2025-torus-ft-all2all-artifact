#! /bin/bash 
SCRIPT_DIR=$(dirname "$(realpath $0)")

# Relative paths to useful directories
BINARY=${SCRIPT_DIR}/../../../build/astra_analytical/build/AnalyticalAstra/bin/AnalyticalAstra
NETWORK_DIR=${SCRIPT_DIR}/../../../inputs/network/analytical/Google_comp
SYSTEM_DIR=${SCRIPT_DIR}/../../../inputs/system/Google_comp
WORKLOAD_DIR=${SCRIPT_DIR}/../../../inputs/workload
STATS_DIR=${SCRIPT_DIR}/../../../examples/results/Google_444

# Ensure the STATS_DIR exists
mkdir -p "${STATS_DIR}"

# Define networks and workloads
NETWORKS=("TPUv4_4x4x4_NoFault.json" "TPUv4_4x4x4_SingleFault.json")
WORKLOADS=("AllToAll_Synthetic_32KB.txt" "AllToAll_Synthetic_64KB.txt" "AllToAll_Synthetic_128KB.txt" "AllToAll_Synthetic_256KB.txt" "AllToAll_Synthetic_512KB.txt" "AllToAll_Synthetic_1MB.txt" "AllToAll_Synthetic_2MB.txt" "AllToAll_Synthetic_4MB.txt" "AllToAll_Synthetic_8MB.txt" "AllToAll_Synthetic_16MB.txt")

# Loop through networks
for network in "${NETWORKS[@]}"; do
    # Determine matching system configurations
    if [[ "$network" == *"NoFault.json" ]]; then
        SYSTEMS=("No_Fault.txt")
    elif [[ "$network" == *"SingleFault.json" ]]; then
        # Two possibilities for SingleFault
        SYSTEMS=("MATE.txt" "MATEe.txt")
    else
        echo "Unknown network type: $network"
        continue
    fi

    # Loop through each system configuration
    for system_file in "${SYSTEMS[@]}"; do
        SYSTEM_CONFIG="${SYSTEM_DIR}/${system_file}"
        system_name="${system_file%.txt}"

        # Set statistics path for this network-system combo
        STATS_PATH="${STATS_DIR}/${network%.json}_${system_name}"
        mkdir -p "${STATS_PATH}"

        # Total experiments for this network-system is the number of workloads
        total_experiments=${#WORKLOADS[@]}
        exp_index=0

        # Loop through workloads
        for workload in "${WORKLOADS[@]}"; do
            WORKLOAD_PATH="${WORKLOAD_DIR}/${workload}"
            run_name="${workload%.txt}_${network%.json}_${system_name}_exp${exp_index}"

            echo "Running experiment: ${run_name}"
            "${BINARY}" \
              --run-name="${run_name}" \
              --network-configuration="${NETWORK_DIR}/${network}" \
              --system-configuration="${SYSTEM_CONFIG}" \
              --workload-configuration="${WORKLOAD_PATH}" \
              --path="${STATS_PATH}/" \
              --num-passes=1 \
              --total-stat-rows=${total_experiments} \
              --stat-row=${exp_index}

            ((exp_index++))
            echo "Experiment ${run_name} completed."
        done

        echo "All experiments for ${network%.*} with ${system_name} completed."
    done
done

echo "All experiments completed."