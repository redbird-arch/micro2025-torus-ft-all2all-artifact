import matplotlib.pyplot as plt
import numpy as np
import csv
from matplotlib.ticker import MultipleLocator

def read_csv_file(file_path):
    with open(file_path, 'r') as file:
        reader = csv.reader(file)
        data = list(reader)
    return data

def reshape_array(array, size=2):
    reshaped = [array[i:i+size] for i in range(0, len(array), size)]
    return reshaped

def process_input_file(file_path):
    data = read_csv_file(file_path)
    arrays = [[], [], []]
    for row in data[1:]:
        run_name = row[0]
        data_value = float(row[1])
        if '2D' in run_name:
            arrays[0].append(data_value)
        elif '3D' in run_name:
            arrays[1].append(data_value)
        elif '4D' in run_name:
            arrays[2].append(data_value)
    return [np.array(reshape_array(arr)) for arr in arrays]

def process_single_data_input_file(file_path):
    data = read_csv_file(file_path)
    array_2D = []
    array_3D = []
    array_4D = []
    for row in data[1:]:
        run_name = row[0]
        data_value = float(row[1])
        if '2D' in run_name:
            array_2D.append(data_value)
        elif '3D' in run_name:
            array_3D.append(data_value)
        elif '4D' in run_name:
            array_4D.append(data_value)
    def reshape_single_array(array):
        reshaped = [[element] for element in array]
        return reshaped
    array_2D = np.array(reshape_single_array(array_2D))
    array_3D = np.array(reshape_single_array(array_3D))
    array_4D = np.array(reshape_single_array(array_4D))
    return array_2D, array_3D, array_4D

def get_matrices(files):
    matrices = {2: [], 3: [], 4: []}
    for i, file_path in enumerate(files):
        if i < 3:  # Process files normally
            array_2D, array_3D, array_4D = process_input_file(file_path)
        else:  # Special processing for other files
            array_2D, array_3D, array_4D = process_single_data_input_file(file_path)
        matrices[2].append(array_2D)
        matrices[3].append(array_3D)
        matrices[4].append(array_4D)
    for index in matrices:
        matrices[index] = np.hstack(matrices[index])
        np.save(f"{index}D.npy", matrices[index])
    return matrices

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

def plot_data(matrices, sizes, chunks):
    fig, axs = plt.subplots(1, 3, figsize=(50, 10))  # Create a 3.5x1 subplot layout
    colors = ['#80ed99', '#06d6a0', '#4cc9f0', '#118ab2', '#fcbf49', '#f77f00', '#c77dff', '#7b2cbf']
    legend_names = ["Ring_Pipeline", "Ring_DimRotation", "HalfRing_Pipeline", "HalfRing_DimRotation",
                    "FoldedRing_Pipeline", "FoldedRing_DimRotation", "MATE", "MATEe"]
    width = 0.1
    titles = [
        'All-to-All Size on 5x5 Torus',
        'All-to-All Size on 5x5x5 Torus',
        'All-to-All Size on 5x5x5x5 Torus'
    ]

    subplot_avgs = []
    for idx, ax in enumerate(axs, start=2):
        speedup_data = matrices[idx][:, 0][:, np.newaxis] / matrices[idx]
        bar_avgs = []
        for i, chunk in enumerate(chunks):
            speedups = [speedup_data[j][i] for j in range(len(sizes))]
            bars = ax.bar(np.arange(len(sizes)) + i * width, speedups, width, label=legend_names[i], color=colors[i], zorder=3)
            bar_avgs.append(np.mean(speedups))  # Calculate mean for this set of bars
        subplot_avgs.append(bar_avgs)
        print(f"Averages for {titles[idx-2]}: {dict(zip(legend_names, bar_avgs))}")

        ax.set_xlabel(titles[idx-2], fontsize=43)
        ax.set_ylabel('Performance Speedup', fontsize=44)
        # y_max = 2.9
        # interval = 0.2  
        # ax.set_yticks(np.arange(0, y_max + interval, interval))
        ax.set_ylim(0.0, 2.85)  
        # ax.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8])  
        ax.set_yticks(np.arange(0, 2.85, 0.4))
        ax.tick_params(axis='y', labelsize=38)
        ax.set_xticks(np.arange(len(sizes)) + width * (len(chunks) / 2 - 0.5))
        ax.set_xticklabels(labels=sizes, fontsize=33)
        ax.set_xticklabels(ax.get_xticklabels(), rotation=40, ha='right')
        ax.yaxis.grid(True, linestyle='-', linewidth=0.5, color='lightgray', zorder=1)
        ax.yaxis.set_major_locator(MultipleLocator(0.4))

    # fig.subplots_adjust(wspace=0.18, bottom=0.17, top=0.8)
    overall_averages = [np.mean([bar_avgs[j] for bar_avgs in subplot_avgs]) for j in range(len(legend_names))]
    print(f"Overall average of averages across all subplots: {dict(zip(legend_names, overall_averages))}")
    # Set legend for the whole figure instead of per subplot
    handles, labels = axs[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc='upper center', fontsize=34, ncol=len(legend_names), bbox_to_anchor=(0.51, 1.023), borderaxespad=0.5, handletextpad=0.25, columnspacing=1.8, edgecolor='black')
    # plt.subplots_adjust(top=0.92, bottom=0.132) 
    plt.tight_layout(rect=[0, 0, 1, 0.93])  # Adjust the layout to make room for the legend
    plt.savefig('Synthetic_Speedup_Exp.png', dpi=600)  # Save the figure as one PNG file
    plt.savefig('Synthetic_Speedup_Exp.pdf')  # Save the figure as one PDF file
    # plt.show()

if __name__ == "__main__":
    sizes = ['8KB', '32KB', '128KB', '512KB', '2MB', '8MB', '32MB', '128MB', '512MB', '2GB']
    chunks = [1, 2, 4, 8, 16, 32, 64, 128]
    files = [
        '../examples/results/Synthetic_exp/No_Fault_Ring/backend_end_to_end.csv',
        '../examples/results/Synthetic_exp/No_Fault_HalfRing/backend_end_to_end.csv',
        '../examples/results/Synthetic_exp/Single_Fault/backend_end_to_end.csv',
        '../examples/results/Synthetic_exp/MATE/backend_end_to_end.csv',
        '../examples/results/Synthetic_exp/MATE_Enhanced/backend_end_to_end.csv'
    ]
    matrices = get_matrices(files)
    plot_data(matrices, sizes, chunks)  # Call the plotting function to create the combined plot