#! /bin/bash
SCRIPT_DIR=$(dirname "$(realpath $0)")

# Relative paths to directories and binaries
BINARY=${SCRIPT_DIR}/../../../build/astra_analytical/build/AnalyticalAstra/bin/AnalyticalAstra
NETWORK_DIR=${SCRIPT_DIR}/../../../inputs/network/analytical/Real_Model
SYSTEM_DIR=${SCRIPT_DIR}/../../../inputs/system/Synthetic_exp/MATE_Enhanced
WORKLOAD_DIR=${SCRIPT_DIR}/../../../inputs/workload/Real_Model
STATS_DIR=${SCRIPT_DIR}/../../../examples/results/Real_Model/MATE_Enhanced

# Ensure the STATS_DIR exists
mkdir -p "${STATS_DIR}"

# Initialize experiment index
exp_index=0

# Define TPU version to system configuration mapping
declare -A system_map=( ["TPUv3"]="2D.txt" ["TPUv4"]="3D.txt" )

# Pre-calculate total number of experiments
total_experiments=0
for workload_file in ${WORKLOAD_DIR}/*_TPUv*.txt; do
    TPU_version=$(echo "$(basename "${workload_file}")" | grep -o "TPUv[34]")
    network_version="TPUv${TPU_version:4}"
    network_count=$(ls ${NETWORK_DIR}/${network_version}_FaultTolerance.json | wc -l)
    ((total_experiments+=network_count))
done

# Process each workload file
for workload_file in ${WORKLOAD_DIR}/*_TPUv*.txt; do
    workload=$(basename "${workload_file}")
    TPU_version=$(echo "${workload}" | grep -o "TPUv[34]")
    network_version="TPUv${TPU_version:4}"

    # Check for a corresponding system file for the TPU version
    system_file="${SYSTEM_DIR}/${system_map[${TPU_version}]}"
    if [[ -f "${system_file}" ]]; then
        system=$(basename "${system_file}")
        dim="${system%.*}"  # Extract dimension part (2D or 3D)

        # Loop through all matching network files, specifically the 'FaultTolerance' files
        for network_file in ${NETWORK_DIR}/${network_version}_FaultTolerance.json; do
            if [[ -f "${network_file}" ]]; then
                network=$(basename "${network_file}")

                # Construct the experiment specific directory name
                experiment_dir="${STATS_DIR}/${workload%.txt}_${network%.json}"
                mkdir -p "${experiment_dir}"  # Ensure this directory exists

                echo "Running experiment: Workload=${workload}, System=${system}, Network=${network}"
                "${BINARY}" \
                  --run-name="${workload%.txt}_${network%.json}" \
                  --network-configuration="${network_file}" \
                  --system-configuration="${system_file}" \
                  --workload-configuration="${workload_file}" \
                  --path="${experiment_dir}/" \
                  --num-passes=1 \
                  --total-stat-rows=1 \
                  --stat-row=0

                echo "Experiment with Workload=${workload}, System=${system}, Network=${network} completed."
                ((exp_index++))
            fi
        done
    fi
done

echo "Total experiments run: $total_experiments"
echo "All experiments completed."
