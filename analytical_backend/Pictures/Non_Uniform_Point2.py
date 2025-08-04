import csv
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize, LinearSegmentedColormap
from matplotlib.cm import ScalarMappable
from matplotlib import font_manager
from matplotlib.colors import LinearSegmentedColormap

# === Path ===
file_no_fault = "../examples/results/Non_Uniform_MoE/MATEe/EndToEnd.csv"
# file_mate     = "../examples/results/Non_Uniform_MoE/MATE/EndToEnd.csv"
mate_base_dir = "../../garnet_backend/examples/results/Non_Uniform_MoE/Single_Fault"

def extract_wg_exposed_comm(path):
    values = []
    with open(path, 'r') as f:
        reader = csv.reader(f)
        rows = list(reader)
        for row in rows:
            if len(row) > 6 and row[6].strip() != "":
                try:
                    # values.append(float(row[6]) * 1000)  # us to ns
                    values.append(float(row[6]))  # us to ns
                except ValueError:
                    continue
    # return values[-32:]
    return values

def extract_mate_wg_exposed_comm(base_dir):
    values = []
    for i in range(0, 33):  
        file_path = f"{base_dir}/{i}/EndToEnd.csv"
        try:
            with open(file_path, 'r') as f:
                reader = csv.reader(f)
                rows = list(reader)
                if len(rows) > 1 and len(rows[1]) > 6 and rows[1][6].strip() != "":
                    # values.append(float(rows[1][6]) * 1000)  # us -> ns
                    values.append(float(rows[1][6]))  # us -> ns
                else:
                    values.append(0.0)  
        except Exception as e:
            print(f"Warning: failed to read {file_path}: {e}")
            values.append(0.0)
    return values

# === derive data ===
Uniform_No_Fault = extract_wg_exposed_comm(file_no_fault)
# Uniform_MATE = extract_wg_exposed_comm(file_mate)
Uniform_MATE = extract_mate_wg_exposed_comm(mate_base_dir)
# layers = list(range(32))
layers = list(range(1, 33))  

# === self-defined color-mapping function ===
def generate_colormap(values, base_cmap_name, start=0.3, end=0.85):
    base_cmap = plt.get_cmap(base_cmap_name)
    truncated_cmap = LinearSegmentedColormap.from_list(
        f"{base_cmap_name}_trunc",
        base_cmap(np.linspace(start, end, 256))
    )
    norm = Normalize(vmin=min(values), vmax=max(values))
    colors = [truncated_cmap(norm(v)) for v in values]
    return colors, norm, truncated_cmap

colors_nf, norm_nf, cmap_nf = generate_colormap(Uniform_No_Fault, 'YlGn', 0.05, 0.9)
colors_mate, norm_mate, cmap_mate = generate_colormap(Uniform_MATE, 'YlOrRd', 0.05, 0.9)

# === get the figure ===
fig, ax = plt.subplots(figsize=(21, 12))
sc_nf = ax.scatter(layers, Uniform_No_Fault[-32:], color=colors_nf[-32:], s=2400, marker='o', edgecolors='black', linewidths=4)
sc_mate = ax.scatter(layers, Uniform_MATE[-32:], color=colors_mate[-32:], s=2400, marker='p', edgecolors='black', linewidths=4)
# add two special points after scatter-point-figure (uniform)
ax.scatter(0, Uniform_No_Fault[0], color='#80ed99', s=5400, marker='*', edgecolors='#06d6a0', linewidths=8, zorder=5)
ax.scatter(0, Uniform_MATE[0], color='#ffc971',  s=5400, marker='*', edgecolors='#f2a65a', linewidths=8, zorder=5)

ax.set_xlabel("MoE Model Layer", fontsize=54)
ax.set_ylabel("All-to-All Time (us)", fontsize=57)

default_font = font_manager.FontProperties(size=54)
bold_font = font_manager.FontProperties(weight='bold', size=42)  
ax.set_xticks([0, 8, 16, 24, 32])
ax.set_xticklabels([
    r"$\bf{Uniform}$",  # Latex-style bold text
    '8', '16', '24', '32'
], fontproperties=default_font)

ax.tick_params(axis='y', labelsize=55)
for tick in [0, 8, 16, 24, 32]:
    ax.axvline(x=tick, linestyle='--', color='lightgray', linewidth=0.8, zorder=0)
ax.grid(axis='y', linestyle='--', alpha=0.5, zorder=0)
ax.set_ylim(5.6, 9.3)
ax.set_yticks(np.arange(6, 9.3, 0.5))

# No-Fault colorbar
cbar_ax_nf = fig.add_axes([0.88, 0.12, 0.018, 0.25])  # x, y, width, height
sm_nf = ScalarMappable(norm=norm_nf, cmap=cmap_nf)
sm_nf.set_array([])
cbar_nf = fig.colorbar(sm_nf, cax=cbar_ax_nf)
cbar_nf.set_label("MATEe", labelpad=12, fontsize=48)
cbar_nf.ax.tick_params(labelsize=42, pad=5)

# MATE colorbar
cbar_ax_mate = fig.add_axes([0.88, 0.6, 0.018, 0.25])
sm_mate = ScalarMappable(norm=norm_mate, cmap=cmap_mate)
sm_mate.set_array([])
cbar_mate = fig.colorbar(sm_mate, cax=cbar_ax_mate)
cbar_mate.set_label("WFR", labelpad=12, fontsize=48)
cbar_mate.ax.tick_params(labelsize=42, pad=5)

plt.subplots_adjust(right=0.86, bottom=0.13) 
plt.savefig("Non_Uniform_Point2.png", dpi=600)
plt.savefig("Non_Uniform_Point2.pdf")
# plt.show()


print("=== The time proportion of Uniform_MATEe compared with the first element of Uniform (index 0) ===")
base_uniform = Uniform_No_Fault[0]
ratios_to_uniform = [v / base_uniform for v in Uniform_No_Fault]
for idx, ratio in enumerate(ratios_to_uniform):
    print(f"Layer {idx}: {ratio:.3f}x")
print(f"→ Proportion Range: {min(ratios_to_uniform):.3f}x ~ {max(ratios_to_uniform):.3f}x")
print(f"→ Average Proportion: {np.mean(ratios_to_uniform):.3f}x")

print("=== The time proportion of Uniform_WFR compared with the first element of Uniform (index 0) ===")
base_uniform = Uniform_MATE[0]
ratios_to_uniform2 = [v / base_uniform for v in Uniform_MATE]
for idx, ratio in enumerate(ratios_to_uniform):
    print(f"Layer {idx}: {ratio:.3f}x")
print(f"→ Proportion Range: {min(ratios_to_uniform2):.3f}x ~ {max(ratios_to_uniform2):.3f}x")
print(f"→ Average Proportion: {np.mean(ratios_to_uniform2):.3f}x")


print("\n=== Uniform_WFR vs. Uniform_MATEe+DR the time radio of each layer(index 1~32) ===")
mate_ratios = []
for i in range(1, min(len(Uniform_MATE), len(Uniform_No_Fault))):
    if Uniform_No_Fault[i] != 0:
        ratio = Uniform_MATE[i] / Uniform_No_Fault[i]
        mate_ratios.append(ratio)
        print(f"Layer {i}: MATE = {Uniform_MATE[i]:.1f}, No_Fault = {Uniform_No_Fault[i]:.1f}, Ratio = {ratio:.3f}x")
    else:
        print(f"Layer {i}: No_Fault time is zero, skipped.")

if Uniform_No_Fault[0] != 0:
    first_ratio = Uniform_MATE[0] / Uniform_No_Fault[0]
    print(f"\n→ Uniform MATE = {Uniform_MATE[0]:.1f}, No_Fault = {Uniform_No_Fault[0]:.1f}, Ratio = {first_ratio:.3f}x")
else:
    print(f"\n→ Invalid data for Uniform MATE")

if mate_ratios:
    print(f"\n→ Average Time Proportion of MATE (index 1~32): {np.mean(mate_ratios):.3f}x")
else:
    print("\n→ No valid MATE vs No_Fault")