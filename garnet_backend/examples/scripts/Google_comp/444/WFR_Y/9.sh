#!/bin/bash
start_time=$(date +%s)
SCRIPT_DIR=$(dirname "$(realpath $0)")

# === Executables and paths ===
BINARY=${SCRIPT_DIR}/../../../../../build/astra_garnet/build/gem5.opt
CONFIG=${SCRIPT_DIR}/../../../../../extern/network_backend/garnet/gem5_astra/configs/example/garnet_synth_traffic.py
NETWORK=${SCRIPT_DIR}/../../../../../inputs/network/garnet/Google_comp/444_y
WORKLOAD=${SCRIPT_DIR}/../../../../../inputs/workload/All_To_All_16MB.txt
SYSTEM=${SCRIPT_DIR}/../../../../../inputs/system/Google.txt
BASE_STATS_DIR=${SCRIPT_DIR}/../../../../../examples/results/Google_comp/444/WFR_Y
SYNTHETIC="--synthetic=training"

# === Set run parameters ===
system_base=9
run_name="Google_comp_444_WFR_Y_${system_base}"
STATS_DIR="${BASE_STATS_DIR}/${system_base}"
mkdir -p "${STATS_DIR}"

# === Run experiment ===
echo "Starting experiment for system file ${system_base}.txt -> run-name: ${run_name}"
experiment_start_time=$(date +%s)
export M5_OUT_DIR="${STATS_DIR}"

"${BINARY}" "${CONFIG}" ${SYNTHETIC} \
    --network-configuration="${NETWORK}" \
    --system-configuration="${SYSTEM}" \
    --workload-configuration="${WORKLOAD}" \
    --path="${STATS_DIR}/" \
    --run-name="${run_name}" \
    --num-passes=1 \
    --total-stat-rows=1 \
    --stat-row=0

# === Final report ===
end_time=$(date +%s)
total_duration=$((end_time - start_time))
days=$((total_duration / 86400))
hours=$(( (total_duration % 86400) / 3600 ))
minutes=$(( (total_duration % 3600) / 60 ))
seconds=$((total_duration % 60))

echo "Experiment 9 completed."
echo "Execution time: ${days} day(s) ${hours} hour(s) ${minutes} minute(s) ${seconds} second(s)"
