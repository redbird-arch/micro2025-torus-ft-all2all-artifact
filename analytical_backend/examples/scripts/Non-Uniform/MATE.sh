#!/bin/bash
SCRIPT_DIR=$(dirname "$(realpath $0)")

# Paths
WORKLOAD_DIR="${SCRIPT_DIR}/../../../inputs/workload/Non-Uniform-MoE"
WORKLOAD_FILE="${WORKLOAD_DIR}/0.txt"
SYSTEM_DIR="${SCRIPT_DIR}/../../../inputs/system/Non-Uniform/MATE"
NETWORK_FILE="${SCRIPT_DIR}/../../../inputs/network/analytical/Google_comp/TPUv4_4x4x4_SingleFault.json"
STATS_DIR="${SCRIPT_DIR}/../../../examples/results/Non_Uniform_MoE/MATE"
BINARY="${SCRIPT_DIR}/../../../build/astra_analytical/build/AnalyticalAstra/bin/AnalyticalAstra"

# Ensure stats directory exists
mkdir -p "${STATS_DIR}"

# Sorted list of 32 system files
SYSTEM_FILES=$(ls "${SYSTEM_DIR}"/*.txt | sort -V)

# Total number of system files
TOTAL_ROWS=33

i=0
for SYSTEM_FILE in ${SYSTEM_FILES}; do
  # run_id=$((i + 1))
  echo "Running workload with system config: $(basename ${SYSTEM_FILE}), stat-row: ${i}"

  "${BINARY}" \
    --run-name="NonUniformMoE_${i}" \
    --network-configuration="${NETWORK_FILE}" \
    --system-configuration="${SYSTEM_FILE}" \
    --workload-configuration="${WORKLOAD_FILE}" \
    --path="${STATS_DIR}/" \
    --num-passes=1 \
    --total-stat-rows=${TOTAL_ROWS} \
    --stat-row=${i}

  echo "Finished run ${i}, system file: $(basename ${SYSTEM_FILE})"
  ((i++))
done
