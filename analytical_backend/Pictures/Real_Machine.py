import matplotlib.pyplot as plt
import numpy as np
import re

# === read the log file and get the average time ===
log_path = '../../real_machine/All_to_All.log'

with open(log_path, 'r') as f:
    log = f.read()

# derive the tensor size 
tensor_size_matches = re.findall(r'Comm-Tensor-Size=\[11520,(\d+)\]', log)
tensor_sizes = [int(s) for s in tensor_size_matches]

# derive average time
baseline_times = [float(x) for x in re.findall(r'\[All2All_Baseline Average Time\] ([0-9.]+)', log)]
halfring_times = [float(x) for x in re.findall(r'\[All2All_HalfRing Average Time\] ([0-9.]+)', log)]
mate_times     = [float(x) for x in re.findall(r'\[All2All_MATE Average Time\] ([0-9.]+)', log)]

assert len(tensor_sizes) == len(baseline_times) == len(halfring_times) == len(mate_times)

# === calculate the bandwidth of each method ===
num_elements = np.array(tensor_sizes)   
total_bytes = num_elements * 4 * 11520  # 4 byte for each data element
total_kb = total_bytes / 1024
total_mb = total_kb / 1024
total_gb = total_mb / 1024


# automatically get the format unit (KB or MB)
def format_bytes(kb_val):
    if kb_val >= 1024:
        return f"{kb_val/1024:.0f}MB"
    else:
        return f"{kb_val:.0f}KB"

bw_baseline = total_gb / np.array(baseline_times)
bw_halfring = total_gb / np.array(halfring_times)
bw_mate     = total_gb / np.array(mate_times)

# === draw ===
Data_Size_Labels = ["22.5MB", "45MB", "90MB", "180MB", "360MB", "720MB", "1.44GB", "2.88GB", "5.76GB"]
x_indices = np.arange(len(Data_Size_Labels))
alternate_labels = [label if i % 2 == 0 else '' for i, label in enumerate(Data_Size_Labels)]


fig, ax = plt.subplots(figsize=(21, 13))
legends = ['Ring+Pipe', 'HalfR+DR', 'MATE']
fill_colors = ['#80ed99', '#4cc9f0', '#ff87ab']
line_colors = ['#06d6a0', '#118ab2', '#ff5d8f']
markers = ['D', 'h', 'v']
linestyles = ['-', '-', '-']

data_arrays = [bw_baseline, bw_halfring, bw_mate]

for i, bandwidth in enumerate(data_arrays):
    ax.plot(x_indices, bandwidth, label=legends[i], marker=markers[i], markersize=40,
            color=line_colors[i], markerfacecolor=fill_colors[i], markeredgecolor=line_colors[i],
            linestyle=linestyles[i], markeredgewidth=10, linewidth=12)

# axis and label
ax.set_xlabel('All-to-All Size on 4x4 Torus', fontsize=60)
ax.set_ylabel('Bandwidth (GB/s)', fontsize=64)
ax.set_xticks(x_indices)
ax.set_xticklabels(alternate_labels)
ax.xaxis.set_label_coords(0.5, -0.12)
ax.tick_params(axis='x', labelsize=56)
ax.tick_params(axis='y', labelsize=64)

ax.set_ylim(0, 7.5)
ax.set_yticks(np.arange(1, 8, 1))
ax.grid(True, linewidth=1.4)

# legend
handles, labels = ax.get_legend_handles_labels()
ax.legend(handles, labels,
          loc='upper center', bbox_to_anchor=(0.5, 1.16),
          handletextpad=0.3, ncol=3, fontsize=48,
          markerscale=1.1, borderpad=0.4, columnspacing=2.0)

plt.subplots_adjust(top=0.855, bottom=0.20)

# save
plt.savefig('Real_Machine.pdf')
plt.savefig('Real_Machine.png', dpi=400)

# === print the speedups ===
print("\n--- Speedup Compared to Baseline ---")
print("Data Size        HalfRing Speedup     MATE Speedup")
for label, base, half, mate in zip(Data_Size_Labels, bw_baseline, bw_halfring, bw_mate):
    print(f"{label:<15} {half/base:>10.3f}x             {mate/base:>10.3f}x")

# === print the speedups ===
halfring_speedup_avg = np.mean(bw_halfring / bw_baseline)
mate_speedup_avg     = np.mean(bw_mate / bw_baseline)

print(f"\nAverage Speedup of HalfRing over Baseline: {halfring_speedup_avg:.3f}x")
print(f"Average Speedup of MATE over Baseline:     {mate_speedup_avg:.3f}x")

