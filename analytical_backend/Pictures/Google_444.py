import os
import pandas as pd
import numpy as np
from scipy.stats import gmean
import matplotlib.pyplot as plt

def read_series_from_fault_tolerance_csv(csv_path):
    """
    Read the 'total exposed comm' column from a single EndToEnd.csv,
    skip the first entry (32KB), and take the next 9 values (64KB-16MB).
    Returns a list of 9 floats.
    """
    df = pd.read_csv(csv_path, header=0)
    series = df['total exposed comm'].iloc[1:10]
    return series.values.tolist()

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
    except pd.errors.EmptyDataError:
        return 0.0

def compute_bandwidth(time_list):
    """
    Compute a bandwidth array (GB/s) from time_list (in microseconds):
    size = 64KB * 2**i; bw = size / time / 1000.
    Returns a numpy array.
    """
    bw = []
    for i, t in enumerate(time_list):
        size_bytes = 64 * 1024 * (2 ** i)
        bw.append(size_bytes / t / 1000.0)
    return np.array(bw)

def compute_bandwidth_with_mask(time_list):
    """
    Compute a bandwidth array (GB/s) from time_list (in microseconds):
    size = 64KB * 2**i; bw = size / time / 1000.
    Returns a numpy array.
    If the CSV file is empty or has no valid data, return 0.
    """
    bw = []
    zero_mask = []
    for i, t in enumerate(time_list):
        if t == 0:
            bw.append(0.0)
            zero_mask.append(True)
        else:
            size_bytes = 64 * 1024 * (2 ** i)
            bw.append(size_bytes / t / 1000.0)
            zero_mask.append(False)
    return np.array(bw), np.array(zero_mask)


# ===== 1. Read HalfRing_DimRotation data =====
half_csv = os.path.join(
    '..', 'examples', 'results', 'Google_444',
    'TPUv4_4x4x4_NoFault_No_Fault', 'EndToEnd.csv'
)
HalfRing_times = read_series_from_fault_tolerance_csv(half_csv)

# ===== 2. Read MATE and MATEe data =====
mate_csv = os.path.join(
    '..', 'examples', 'results', 'Google_444',
    'TPUv4_4x4x4_SingleFault_MATE', 'EndToEnd.csv'
)
matee_csv = os.path.join(
    '..', 'examples', 'results', 'Google_444',
    'TPUv4_4x4x4_SingleFault_MATEe', 'EndToEnd.csv'
)
MATE_times  = read_series_from_fault_tolerance_csv(mate_csv)
MATEe_times = read_series_from_fault_tolerance_csv(matee_csv)

# ===== 3. Read DORMIN (DOR) data =====
base_dor = os.path.join(
    '..', '..', 'garnet_backend', 'examples', 'results',
    'Google_comp', '444', 'DOR'
)
DORMIN_times = []
for i in range(1, 10):
    csv_path = os.path.join(base_dor, str(i), 'EndToEnd.csv')
    DORMIN_times.append(read_single_time(csv_path))

# ===== 4. Read WFR_1, WFR_2, WFR_3 data =====
wfr_bases = {
    'WFR_1': 'WFR_X',
    'WFR_2': 'WFR_Y',
    'WFR_3': 'WFR_Z',
}
WFR_times = {}
for key, folder in wfr_bases.items():
    times = []
    base = os.path.join(
        '..', '..', 'garnet_backend', 'examples', 'results',
        'Google_comp', '444', folder
    )
    for i in range(1, 10):
        csv_path = os.path.join(base, str(i), 'EndToEnd.csv')
        times.append(read_single_time(csv_path))
    WFR_times[key] = times

# ===== 5. Compute bandwidth arrays =====
DORMIN, DORMIN_zero_mask = compute_bandwidth_with_mask(DORMIN_times)
HalfRing_DimRotation     = compute_bandwidth(HalfRing_times)
WFR_1, WFR_1_zero_mask   = compute_bandwidth_with_mask(WFR_times['WFR_1'])
WFR_2, WFR_2_zero_mask   = compute_bandwidth_with_mask(WFR_times['WFR_2'])
WFR_3, WFR_3_zero_mask   = compute_bandwidth_with_mask(WFR_times['WFR_3'])
WFR                      = (WFR_1 + WFR_2 + WFR_3) / 3.0
MATE                     = compute_bandwidth(MATE_times)
MATEe                    = compute_bandwidth(MATEe_times)

# ===== 6. (Optional) Print average speedup values =====
def safe_speedup(numerator, denominator):
    """Compute elementwise speedup only where denominator > 0."""
    numerator = np.array(numerator, dtype=float)
    denominator = np.array(denominator, dtype=float)
    mask = denominator > 0
    speedup = np.zeros_like(numerator)
    speedup[mask] = numerator[mask] / denominator[mask]
    valid_values = speedup[mask & (numerator > 0)]
    return speedup, valid_values

speedup_HalfRing_DORMIN, valid1 = safe_speedup(HalfRing_DimRotation, DORMIN)
speedup_MATE_WFR,        valid2 = safe_speedup(MATE, WFR)
speedup_MATEe_WFR,       valid3 = safe_speedup(MATEe, WFR)
slowdown_WFR_DORMIN,     valid4 = safe_speedup(WFR, DORMIN)

print("Average speedup HalfRing vs DORMIN:", gmean(valid1) if len(valid1) else 0.0)
print("Average speedup MATE vs WFR:",        gmean(valid2) if len(valid2) else 0.0)
print("Average speedup MATEe vs WFR:",       gmean(valid3) if len(valid3) else 0.0)
print("Average slowdown WFR vs DORMIN:",     gmean(valid4) if len(valid4) else 0.0)

# ===== 7. Plotting section (identical to the original) =====
Data_Size_Labels = ['64KB', '128KB', '256KB', '512KB', '1MB', '2MB', '4MB', '8MB', '16MB']
x_indices = np.arange(len(Data_Size_Labels))

fig, ax = plt.subplots(figsize=(21, 13))
legends     = ['DOR', 'HalfR+DR', 'WFR-x', 'WFR-y', 'WFR-z', 'MATE', 'MATEe']
fill_colors = ['#80ed99', '#4cc9f0', '#ffc971', '#f79d65', '#db7c26', '#ff87ab', '#c77dff']
line_colors = ['#06d6a0', '#118ab2', '#f2a65a', '#f4845f', '#d8572a', '#ff5d8f', '#7b2cbf']
linestyles  = ['-'] * 7
markers     = ['o', 'D', 's', 'p', 'h', '^', 'v']
data_arrays = [DORMIN, HalfRing_DimRotation, WFR_1, WFR_2, WFR_3, MATE, MATEe]
zero_masks  = [DORMIN_zero_mask, None, WFR_1_zero_mask, WFR_2_zero_mask, WFR_3_zero_mask, None, None]

for i, (bandwidth, zero_mask) in enumerate(zip(data_arrays, zero_masks)):
    ax.plot(x_indices, bandwidth, label=legends[i], marker=markers[i], markersize=22,
            color=line_colors[i], markerfacecolor=fill_colors[i],
            markeredgecolor=line_colors[i], linestyle='-', markeredgewidth=7, linewidth=7)
    if zero_mask is not None and zero_mask.any():
        ax.scatter(x_indices[zero_mask], bandwidth[zero_mask], color='gray', s=350, zorder=5, marker=markers[i])


ax.set_xlabel('All-to-All Size on 4x4x4 Torus', fontsize=55)
ax.set_ylabel('Bandwidth (GB/s)', fontsize=55)
ax.xaxis.set_label_coords(0.5, -0.12)
ax.tick_params(axis='x', labelsize=50)
ax.tick_params(axis='y', labelsize=52)
ax.set_xticks(x_indices)
alternate_labels = [label if i % 2 == 0 else '' for i, label in enumerate(Data_Size_Labels)]
ax.set_xticklabels(alternate_labels)
ax.set_ylim(15, 125)
ax.set_yticks(np.arange(20, 125, 20))
ax.grid(True, linewidth=1.4)
ax.legend(loc='upper center', bbox_to_anchor=(0.5, 1.23),
          handletextpad=0.16, ncol=4, fontsize=44,
          markerscale=1.2, borderpad=0.15, columnspacing=0.8)
plt.subplots_adjust(top=0.87, bottom=0.20)
plt.savefig('Google_444_IBM.pdf')
plt.savefig('Google_444_IBM.png', dpi=400)
