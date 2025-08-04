import os
import pandas as pd
import numpy as np
from scipy.stats import gmean
import matplotlib.pyplot as plt

def read_series_from_csv(csv_path):
    """
    Read the 'total exposed comm' column from a single EndToEnd.csv,
    skip the first entry (32KB), and take the next 9 values (64KB-16MB).
    Returns a list of 9 floats.
    """
    try:
        df = pd.read_csv(csv_path, header=0)
        if 'total exposed comm' not in df.columns:
            return [0.0] * 9
        series = df['total exposed comm'].iloc[1:10]
        values = series.values.tolist()
        if len(values) < 9:
            values += [0.0] * (9 - len(values))
        return values
    except (FileNotFoundError, pd.errors.EmptyDataError):
        return [0.0] * 9

def read_single_time(csv_path):
    """
    Read the 'total exposed comm' value from the first record of a single EndToEnd.csv.
    If the CSV file is empty or has no valid data, return 0.
    """
    try:
        df = pd.read_csv(csv_path, header=0)
        if 'total exposed comm' not in df.columns or df['total exposed comm'].dropna().empty:
            return 0.0
        return df['total exposed comm'].iloc[0]
    except (FileNotFoundError, pd.errors.EmptyDataError):
        return 0.0

def compute_bandwidth(time_list):
    """
    Compute a bandwidth array (GB/s) from time_list (in microseconds):
    size = 64KB * 2**i; bw = size / time / 1000.
    Returns a numpy array.
    """
    bw = []
    for i, t in enumerate(time_list):
        if t == 0:
            bw.append(0.0)
        else:
            size_bytes = 64 * 1024 * (2 ** i)
            bw.append(size_bytes / t / 1000.0)
    return np.array(bw)

# ===== 1. Read MATE/MATEe (Ours) data =====
ours_paths = {
    'Failure1_Ours': os.path.join('..', 'examples', 'results', 'Google_MultiFault', 'Fault_2_3', 'EndToEnd.csv'),
    'Failure2_Ours': os.path.join('..', 'examples', 'results', 'Google_MultiFault', 'Fault_2_1', 'EndToEnd.csv'),
    'Failure3_Ours': os.path.join('..', 'examples', 'results', 'Google_MultiFault', 'Fault_2_5', 'EndToEnd.csv'),
}
Ours_times = {k: read_series_from_csv(path) for k, path in ours_paths.items()}

# ===== 2. Read Google (WFR) data from multiple folders =====
google_fault_types = ['Type1', 'Type2', 'Type3']
Google_times = {}

for idx, fault_type in enumerate(google_fault_types, start=1):
    key = f'Failure{idx}_Google'
    times = []
    base = os.path.join(
        '..', '..', 'garnet_backend', 'examples', 'results',
        'Google_MultiFault', fault_type
    )
    for i in range(1, 10):
        csv_path = os.path.join(base, str(i), 'EndToEnd.csv')
        times.append(read_single_time(csv_path))
    Google_times[key] = times

# ===== 3. Compute bandwidth arrays =====
Failure1_Ours   = compute_bandwidth(Ours_times['Failure1_Ours'])
Failure2_Ours   = compute_bandwidth(Ours_times['Failure2_Ours'])
Failure3_Ours   = compute_bandwidth(Ours_times['Failure3_Ours'])
Failure1_Google = compute_bandwidth(Google_times['Failure1_Google'])
Failure2_Google = compute_bandwidth(Google_times['Failure2_Google'])
Failure3_Google = compute_bandwidth(Google_times['Failure3_Google'])
# ===== 4. Compute speedups and geometric mean =====
def compute_speedup(ours, google):
    """Compute elementwise speedup and geometric mean, skip zeros."""
    ours = np.array(ours, dtype=float)
    google = np.array(google, dtype=float)
    mask = google > 0
    speedup = np.zeros_like(ours)
    speedup[mask] = ours[mask] / google[mask]
    valid = speedup[mask & (ours > 0)]
    return speedup, gmean(valid) if len(valid) else 0.0

speedup_1, avg1 = compute_speedup(Failure1_Ours, Failure1_Google)
speedup_2, avg2 = compute_speedup(Failure2_Ours, Failure2_Google)
speedup_3, avg3 = compute_speedup(Failure3_Ours, Failure3_Google)

print("Average speedup (Failure1):", avg1)
print("Average speedup (Failure2):", avg2)
print("Average speedup (Failure3):", avg3)

# ===== 5. Plotting section =====
Data_Size_Labels = ['64KB', '128KB', '256KB', '512KB', '1MB', '2MB', '4MB', '8MB', '16MB']
x_indices = np.arange(len(Data_Size_Labels))

fig, ax = plt.subplots(figsize=(21, 13))
legends = ['WFR-Type1', 'WFR-Type2', 'WFR-Type3', 'MATE-Type1', 'MATE-Type2', 'MATEe-Type3']
fill_colors = ['#ffe5ec', '#caf0f8', '#ffc971', '#ff87ab', '#00b4d8', '#db7c26']
line_colors = ['#ffc2d1', '#48cae4', '#f2a65a', '#ff5d8f', '#118ab2', '#d8572a']
markers = ['D', 'h', 'v', 'o', 's', '^']
linestyles = ['-', '-', '-', '-', '-', '-']

data_arrays = [Failure1_Google, Failure2_Google, Failure3_Google,
               Failure1_Ours, Failure2_Ours, Failure3_Ours]

# Plot all lines
for i, bandwidth in enumerate(data_arrays):
    ax.plot(x_indices, bandwidth, label=legends[i], marker=markers[i], markersize=25,
            color=line_colors[i], markerfacecolor=fill_colors[i], markeredgecolor=line_colors[i],
            linestyle=linestyles[i], markeredgewidth=7, linewidth=9)

# Axis labels and ticks
ax.set_xlabel('All-to-All Size on 4x4x4 Torus', fontsize=56)
ax.set_ylabel('Bandwidth (GB/s)', fontsize=58)
ax.xaxis.set_label_coords(0.5, -0.12)
ax.tick_params(axis='x', labelsize=53)
ax.tick_params(axis='y', labelsize=55)
ax.set_xticks(x_indices)
alternate_labels = [label if i % 2 == 0 else '' for i, label in enumerate(Data_Size_Labels)]
ax.set_xticklabels(alternate_labels)

# Y-axis range and ticks
ax.set_ylim(8, 95)
ax.set_yticks(np.arange(20, 95, 10))
ax.grid(True, linewidth=1.4)

# Legend reorder
handles, labels = ax.get_legend_handles_labels()
desired_order = [3, 0, 4, 1, 5, 2]
ordered_handles = [handles[i] for i in desired_order]
ordered_labels = [labels[i] for i in desired_order]

ax.legend(ordered_handles, ordered_labels,
          loc='upper center', bbox_to_anchor=(0.5, 1.25),
          handletextpad=0.2, ncol=3, fontsize=44,
          markerscale=1.05, borderpad=0.15, columnspacing=1.0)

plt.subplots_adjust(top=0.855, bottom=0.20)
plt.savefig('Google_MultiFault.pdf')
plt.savefig('Google_MultiFault.png', dpi=400)