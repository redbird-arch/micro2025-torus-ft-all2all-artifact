import matplotlib.pyplot as plt
import numpy as np

def load_and_process_data(file_name, dimension):
    data = np.load(file_name)
    Time = np.delete(data, [0, 2, 4], axis=1)  # delete 1, 3, 5 elements
    Bandwidth = np.zeros_like(Time, dtype=float)
    factor = (5**dimension - 1) / (5**dimension)
    # factor = 1
    for i in range(len(Time)):
        bandwidth = (Data_Size[i] * factor) / Time[i]
        Bandwidth[i] = np.round(bandwidth, 3)
    return Bandwidth

# Data size array and labels
Data_Size = np.array([8, 32, 128, 512, 2048, 8192, 32768, 131072, 524288, 2097152])
Data_Size_Labels = ['8KB', '32KB', '128KB', '512KB', '2MB', '8MB', '32MB', '128MB', '512MB', '2GB']

# Setup plot
fig, axes = plt.subplots(1, 3, figsize=(40, 7))
legends = ['Ring', 'HalfRing', 'FoldedRing', 'MATE', 'MATEe']
fill_colors = ['#80ed99', '#4cc9f0', '#fcbf49', '#ff87ab', '#c77dff']
line_colors = ['#06d6a0', '#118ab2', '#f77f00', '#ff5d8f', '#7b2cbf']
linestyles = ['-', '-', '-', '-', '-']
markers = ['o', 'D', 's', '^', 'v']
x_indices = np.arange(len(Data_Size))

titles = ['5x5 Torus', '5x5x5 Torus', '5x5x5x5 Torus']
dimensions = [2, 3, 4]

for ax, file_name, title, dim in zip(axes, ['2D.npy', '3D.npy', '4D.npy'], titles, dimensions):
    Bandwidth = load_and_process_data(file_name, dim)
    for i in range(Bandwidth.shape[1]):
        ax.plot(x_indices, Bandwidth[:, i], label=legends[i] if ax is axes[0] else "", marker=markers[i], markersize=14,
                color=line_colors[i], markerfacecolor=fill_colors[i], markeredgecolor=line_colors[i],
                linestyle=linestyles[i], markeredgewidth=3, linewidth=3)
    ax.set_xlabel('All-to-All Size on ' + title, fontsize=30)
    ax.set_ylabel('Bandwidth (GB/s)', fontsize=34)
    # ax.tick_params(axis='both', labelsize=18)
    ax.tick_params(axis='x', labelsize=26, rotation=35)
    ax.tick_params(axis='y', labelsize=29)
    ax.set_xticks(x_indices)
    ax.set_xticklabels(Data_Size_Labels)
    # y_max = Bandwidth.max()
    # y_max = 60
    # interval = 5  
    # ax.set_yticks(np.arange(0, y_max + interval, interval))
    ax.set_ylim(0, 60)  
    ax.set_yticks([0, 10, 20, 30, 40, 50, 60]) 
    ax.grid(True)

# Adjust subplots close to each other
fig.subplots_adjust(wspace=0.17, bottom=0.22, top=0.9)


# Add a single legend for the whole figure
handles, labels = axes[0].get_legend_handles_labels()
fig.legend(handles, labels, loc='upper center', ncol=len(legends), fontsize=28, bbox_to_anchor=(0.5, 1.019), handletextpad=0.35, columnspacing=6.0, borderpad=0.2, markerscale=1.2)

plt.savefig('Synthetic_Bandwidth_Exp.png', dpi=600)  # Save the figure as one PNG file
plt.savefig('Synthetic_Bandwidth_Exp.pdf')  # Save the figure as one PDF file
# plt.show()