#!/bin/bash

echo "======== Begin experiments ========"
# ==============================================
# Synthetic Experiments (Figure 11,12,13)
# ==============================================
echo $(date +%F/%T)
echo "[START] Exp 1: Running synthetic experiments for Figure 11,12,13..."
bash Run_Synthetic.sh > Synthetic.log 2>&1
echo "[END]   Exp 1: Synthetic experiments completed."
echo $(date +%F/%T)
# ==============================================
# Scalability Experiments (Figure 14)
# ==============================================
echo $(date +%F/%T)
echo "[START] Exp 2: Running scalability experiments for Figure 14..."
bash Run_Scalability.sh > Scalability.log 2>&1
echo "[END]   Exp 2: Scalability experiments completed."
echo $(date +%F/%T)
# ==============================================
# Google Comparison Experiments (Figure 15 analytical part)
# ==============================================
echo $(date +%F/%T)
echo "[START] Exp 3: Running Google comparison experiments for Figure 15..."
bash Run_Google_comp.sh > Google_comp.log 2>&1
echo "[END]   Exp 3: Google comparison experiments completed."
echo $(date +%F/%T)
# ==============================================
# Real Model Experiments (Figure 16)
# ==============================================
echo $(date +%F/%T)
echo "[START] Exp 4: Running real model experiments for Figure 16..."
bash Run_Real_Model.sh > Real_Model.log 2>&1
echo "[END]   Exp 4: Real Model experiments completed."
echo $(date +%F/%T)
# ==============================================
# Non-Uniform Experiments (Figure 17 analytical part)
# ==============================================
echo $(date +%F/%T)
echo "[START] Exp 5: Running non-uniform experiments for Figure 17..."
bash Run_Non-Uniform.sh > Non-Uniform.log 2>&1
echo "[END]   Exp 5: Non-Uniform experiments completed."
echo $(date +%F/%T)
# ==============================================
# Multi Failures Experiments (Figure 18(b) analytical part)
# ==============================================
echo $(date +%F/%T)
echo "[START] Exp 6: Running multi failures experiments for Figure 18(b)..."
bash Run_Google_multifault.sh > Google_multifault.log 2>&1
echo "[END]   Exp 6: Multi Failures experiments completed."
echo $(date +%F/%T)
echo "======== All the experiments completed ========"