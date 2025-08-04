#!/bin/bash


# # ============== Google_comp part (844) ==============

# TEMPLATE_SCRIPT="./Google_comp/844/DOR/1.sh"
# OUTPUT_DIR="./Google_comp/844/DOR"
# TEMPLATE_SCRIPT="./Google_comp/844/WFR_F1/1.sh"
# OUTPUT_DIR="./Google_comp/844/WFR_F1"
TEMPLATE_SCRIPT="./Google_comp/844/WFR_F2/1.sh"
OUTPUT_DIR="./Google_comp/844/WFR_F2"

if [[ ! -f "${TEMPLATE_SCRIPT}" ]]; then
    echo "ERROR: Template script ${TEMPLATE_SCRIPT} not found."
    exit 1
fi

workload_files=(
    "All_To_All_64KB.txt"
    "All_To_All_128KB.txt"
    "All_To_All_256KB.txt"
    "All_To_All_512KB.txt"
    "All_To_All_1MB.txt"
    "All_To_All_2MB.txt"
    "All_To_All_4MB.txt"
    "All_To_All_8MB.txt"
    "All_To_All_16MB.txt"
)

# === generate 2.sh to 9.sh ===
for i in $(seq 2 9); do
    idx=$((i - 1)) 
    target="${OUTPUT_DIR}/${i}.sh"
    cp "${TEMPLATE_SCRIPT}" "${target}"

    sed -i "s|WORKLOAD=.*|WORKLOAD=\${SCRIPT_DIR}/../../../../../inputs/workload/${workload_files[$idx]}|g" "${target}"
    sed -i "s|system_base=.*|system_base=${i}|g" "${target}"
    sed -i "s|Experiment [0-9]* completed.|Experiment ${i} completed.|g" "${target}"
    
    if [[ $i -ge 6 ]]; then
        sed -i "s|NETWORK=.*|NETWORK=\${SCRIPT_DIR}/../../../../../inputs/network/garnet/Google_comp/844_2|g" "${target}"
    fi
    chmod +x "${target}"
done

echo "Generated scripts: 2.sh to 9.sh"








# # ============== Google_comp part (444) ==============
# TEMPLATE_SCRIPT="./Google_comp/444/WFR_X/1.sh"
# OUTPUT_DIR="./Google_comp/444/WFR_X"
# TEMPLATE_SCRIPT="./Google_comp/444/WFR_Y/1.sh"
# OUTPUT_DIR="./Google_comp/444/WFR_Y"
# TEMPLATE_SCRIPT="./Google_comp/444/WFR_Z/1.sh"
# OUTPUT_DIR="./Google_comp/444/WFR_Z"
# TEMPLATE_SCRIPT="./Google_comp/444/DOR/1.sh"
# OUTPUT_DIR="./Google_comp/444/DOR"

# # ============== Google_MultiFault part ==============
# TEMPLATE_SCRIPT="./Google_MultiFault/Type1/1.sh"
# OUTPUT_DIR="./Google_MultiFault/Type1"
# TEMPLATE_SCRIPT="./Google_MultiFault/Type2/1.sh"
# OUTPUT_DIR="./Google_MultiFault/Type2"
# TEMPLATE_SCRIPT="./Google_MultiFault/Type3/1.sh"
# OUTPUT_DIR="./Google_MultiFault/Type3"

# if [[ ! -f "${TEMPLATE_SCRIPT}" ]]; then
#     echo "ERROR: Template script ${TEMPLATE_SCRIPT} not found."
#     exit 1
# fi

# workload_files=(
#     "All_To_All_64KB.txt"
#     "All_To_All_128KB.txt"
#     "All_To_All_256KB.txt"
#     "All_To_All_512KB.txt"
#     "All_To_All_1MB.txt"
#     "All_To_All_2MB.txt"
#     "All_To_All_4MB.txt"
#     "All_To_All_8MB.txt"
#     "All_To_All_16MB.txt"
# )

# # === generate 2.sh to 9.sh ===
# for i in $(seq 2 9); do
#     idx=$((i - 1)) 
#     target="${OUTPUT_DIR}/${i}.sh"
#     cp "${TEMPLATE_SCRIPT}" "${target}"

#     sed -i "s|WORKLOAD=.*|WORKLOAD=\${SCRIPT_DIR}/../../../../inputs/workload/${workload_files[$idx]}|g" "${target}"
#     # sed -i "s|WORKLOAD=.*|WORKLOAD=\${SCRIPT_DIR}/../../../../../inputs/workload/${workload_files[$idx]}|g" "${target}"
#     sed -i "s|system_base=.*|system_base=${i}|g" "${target}"
#     sed -i "s|Experiment [0-9]* completed.|Experiment ${i} completed.|g" "${target}"

#     chmod +x "${target}"
# done

# echo "Generated scripts: 2.sh to 9.sh"





# # ============== Non-Uniform MoE part ==============
# #!/bin/bash

# TEMPLATE_SCRIPT="./Non-Uniform/No_Fault/0.sh"
# # TEMPLATE_SCRIPT="./Non-Uniform/Single_Fault/0.sh"
# OUTPUT_DIR="./Non-Uniform/No_Fault"
# # OUTPUT_DIR="./Non-Uniform/Single_Fault"

# # === Check template existence ===
# if [[ ! -f "${TEMPLATE_SCRIPT}" ]]; then
#     echo "ERROR: Template script ${TEMPLATE_SCRIPT} not found."
#     exit 1
# fi

# for i in $(seq 0 32); do
#     target="${OUTPUT_DIR}/${i}.sh"
#     cp "${TEMPLATE_SCRIPT}" "${target}"

#     sed -i "s|SYSTEM=.*|SYSTEM=\${SCRIPT_DIR}/../../../../inputs/system/Non-Uniform/${i}.txt|g" "${target}"
#     sed -i "s|system_base=.*|system_base=${i}|g" "${target}"
#     sed -i "s|Experiment [0-9]* completed.|Experiment ${i} completed.|g" "${target}"

#     chmod +x "${target}"
# done

# echo "Generated 33 scripts: ${OUTPUT_DIR}/0.sh to ${OUTPUT_DIR}/32.sh"