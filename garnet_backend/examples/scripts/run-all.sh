#!/bin/bash

echo "======== Begin experiments ========"
# ==============================================
# Google Routing method on 4x4x4 single TPUv4 pod (Figure 15(a) garnet part)
# ==============================================
echo $(date +%F/%T)
echo "[START] Exp 1: Running Google_comp_444 experiments for Figure 15(a)..."
bash Run_Google_comp_444.sh > Google_comp_444.log 2>&1
echo "[END]   Exp 1: Google_comp_444 experiments completed."
echo $(date +%F/%T)
# ==============================================
# Google Routing method on 8x4x4 double TPUv4 pod (Figure 15(b) garnet part)
# ==============================================
echo $(date +%F/%T)
echo "[START] Exp 2: Running Google_comp_844 experiments for Figure 15(b)..."
bash Run_Google_comp_844_1.sh > Google_comp_844_1.log 2>&1
bash Run_Google_comp_844_2.sh > Google_comp_844_2.log 2>&1
bash Run_Google_comp_844_3.sh > Google_comp_844_3.log 2>&1
echo "[END]   Exp 2: Google_comp_844 experiments completed."
echo $(date +%F/%T)
# ==============================================
# Non-Uniform Experiments (Figure 17 garnet part)
# ==============================================
echo $(date +%F/%T)
echo "[START] Exp 3: Running non-uniform experiments for Figure 17..."
bash Run_Non-Uniform.sh > Non-Uniform.log 2>&1
echo "[END]   Exp 3: Non-Uniform experiments completed."
echo $(date +%F/%T)
# ==============================================
# Multi Failures Experiments (Figure 18(b) garnet part)
# ==============================================
echo $(date +%F/%T)
echo "[START] Exp 4: Running multi failures experiments for Figure 18(b)..."
bash Run_Google_MultiFault.sh > Google_MultiFault.log 2>&1
echo "[END]   Exp 4: Multi Failures experiments completed."
echo $(date +%F/%T)
echo "======== All the experiments completed ========"