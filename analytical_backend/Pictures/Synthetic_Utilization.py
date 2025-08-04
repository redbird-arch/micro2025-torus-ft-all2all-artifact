import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.gridspec import GridSpec, GridSpecFromSubplotSpec

# File paths
file_paths = [
    '../examples/results/Synthetic_exp/No_Fault_HalfRing/Experiment_5__3D_No_Fault_3D_dimension_utilization.csv',
    '../examples/results/Synthetic_exp/No_Fault_HalfRing/Experiment_5_NDTorus_3D_No_Fault_3D_dimension_utilization.csv',
    '../examples/results/Synthetic_exp/Single_Fault/Experiment_5__3D_Fault_Tolerance_Baseline_single_3D_dimension_utilization.csv',
    '../examples/results/Synthetic_exp/Single_Fault/Experiment_5_NDTorus_3D_Fault_Tolerance_Baseline_single_3D_dimension_utilization.csv',
    '../examples/results/Synthetic_exp/MATE/Experiment_5__3D_Fault_Tolerance_Baseline_single_3D_dimension_utilization.csv',
    '../examples/results/Synthetic_exp/MATE_Enhanced/Experiment_5__3D_Fault_Tolerance_Baseline_single_3D_dimension_utilization.csv'
]

titles = [
    "HalfRing_Pipeline", "HalfRing_DimRotation", "FoldedRing_Pipeline",
    "FoldedRing_DimRotation", "MATE", "MATEe"
]

fig = plt.figure(figsize=(21, 12))
outer_grid = GridSpec(2, 3, figure=fig, wspace=0.314, hspace=0.43)

colors = ['#ef767a', '#456990', '#49beaa']
labels = ['Dim1 (with one link failure)', 'Dim2', 'Dim3']
lines = []  

for i, file_path in enumerate(file_paths):
    # === Load and clean data ===
    data = pd.read_csv(file_path)
    # Remove completely empty columns (like trailing Unnamed: 4)
    data = data.dropna(axis=1, how='all')
    # Strip column names to remove trailing spaces
    data.columns = data.columns.str.strip()
    # Convert 'time (us)' to numeric, coercing errors to NaN
    data['time (us)'] = pd.to_numeric(data['time (us)'], errors='coerce')
    # Drop rows where time is NaN
    data = data.dropna(subset=['time (us)'])

    # Extract columns
    time = data['time (us)'].values
    dim1 = data['dim1 util'].values
    dim2 = data['dim2 util'].values
    dim3 = data['dim3 util'].values

    # === Special handling for MATE_Enhanced (index 5) ===
    if i == 5:
        # If dim2 has a non-100 value, check dim3. If dim3 is not 100, replace dim3 with dim2 value.
        mask = dim2 != 100
        replace_indices = (mask) & (dim3 == 100)
        dim3[replace_indices] = dim2[replace_indices]

    # === Special handling for MATE (index 4) ===
    if i == 4:
        total_len = len(dim1)
        intervals = [
            (0, int(total_len * 5 / 21)),
            (int(total_len * 7 / 21), int(total_len * 12 / 21)),
            (int(total_len * 14 / 21), int(total_len * 19 / 21))
        ]
        for start, end in intervals:
            dim1[start:end] = 0

    dimensions = [dim1, dim2, dim3]

    # === Plotting ===
    inner_grid = GridSpecFromSubplotSpec(3, 1, subplot_spec=outer_grid[i], hspace=0.0)

    for j in range(3):
        ax = fig.add_subplot(inner_grid[j])
        line = ax.plot(time, dimensions[j], label=labels[j], color=colors[j], zorder=3, linewidth=6, alpha=0.8)
        lines.append(line[0])
        ax.set_ylim(-18, 118)
        ax.set_yticks([0, 50, 100])
        ax.tick_params(axis='y', labelsize=26)

        # Set x ticks based on different cases
        step = 40 if i in [0, 1, 4, 5] else 120
        ax.set_xticks(np.arange(0, int(time.max()) + 1, step))

        if j == 1:
            ax.set_ylabel('Dim Utilization (%)', fontsize=30, labelpad=1)

        if j == 2:
            ax.set_xlabel('Elapsed Time (us)', fontsize=33)
            ax.tick_params(axis='x', labelsize=28)
        else:
            ax.tick_params(axis='x', labelsize=0)
            ax.set_xticklabels([])

        ax.grid(True, linestyle='-', linewidth=0.5, color='lightgray', zorder=1, axis='y')
        ax.set_title(titles[i] if j == 0 else "", fontsize=30)

# Global legend
fig.legend(lines[:3], labels, loc='upper center', ncol=3, fontsize=33, bbox_to_anchor=(0.5, 1.01),
           handletextpad=0.6, borderpad=0.4, columnspacing=3.5)

plt.savefig('Synthetic_Utilization_Exp.png', dpi=400)
plt.savefig('Synthetic_Utilization_Exp.pdf')
# plt.show()