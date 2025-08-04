import matplotlib.pyplot as plt
import csv
import numpy as np
from matplotlib.gridspec import GridSpec
from matplotlib.ticker import MultipleLocator

def read_csv_file_1(file_path):
    with open(file_path, 'r') as file:
        reader = csv.DictReader(file)
        comms_time = {'2D': [], '3D': [], '4D': []}
        for row in reader:
            run_name = row['RunName']
            # print (run_name)
            if '2D' in run_name:
                comms_time['2D'].append(float(row['CommsTime']))
            elif '3D' in run_name:
                comms_time['3D'].append(float(row['CommsTime']))
            elif '4D' in run_name:
                comms_time['4D'].append(float(row['CommsTime']))
        # print (comms_time)
    return comms_time

def read_csv_file_2(file_path):
    with open(file_path, 'r') as file:
        reader = csv.DictReader(file)
        comms_time = [float(row['CommsTime']) for row in reader]
    return comms_time

def process_input_files_1(files):
    data_arrays = []
    for file_path in files:
        data = read_csv_file_1(file_path)
        data_arrays.append(data)
    return data_arrays

def process_input_files_2(files):
    data_arrays = []
    for file_path in files:
        data = read_csv_file_2(file_path)
        data_arrays.append(data)
    return data_arrays

def plot_data_1(ax, data_arrays):
    dimensions = ['4', '6', '8', '10', '12', '14', '16', '18', '20']
    labels = ['Ring', 'HalfRing', 'FoldedRing', 'MATE', 'MATEe']
    fill_colors = ['#80ed99', '#4cc9f0', '#fcbf49', '#fae0e4', '#fbb1bd', '#ff477e', '#e6f2ff', '#b3beff', '#8b79d9']
    line_colors = ['#06d6a0', '#118ab2', '#f77f00', '#f7cad0', '#ff99ac', '#ff0a54', '#ccdcff', '#9a99f2', '#805ebf']
    markers = ['o', 'D', 's', '^', 'v']

    for i, data in enumerate(data_arrays[:3]):
        ax.plot(dimensions, data['2D'], label=labels[i], marker=markers[i], markersize=18, color=line_colors[i], fillstyle='full', markerfacecolor=fill_colors[i], zorder=3, markeredgewidth=2, linewidth=4)

    new_labels = ['MATE-2D', 'MATE-3D', 'MATE-4D', 'MATEe-2D', 'MATEe-3D', 'MATEe-4D']
    for i, data in enumerate(data_arrays[3:]):
        # print(f"i = {i}")
        # if (i != 0):
        for j, (dim, dim_label) in enumerate(zip(['2D', '3D', '4D'], ['2D', '3D', '4D'])):
            # print(f"j = {j}")
            ax.plot(dimensions, data[dim], label=new_labels[i*3+j], marker=markers[i+3], markersize=19, color=line_colors[3+i*3+j], fillstyle='full', markerfacecolor=fill_colors[3+i*3+j], zorder=3, markeredgewidth=2, linewidth=6)

    ax.set_xlabel('Node Number per Dimension', fontsize=60)
    ax.set_ylabel('All-to-All Time (us)', fontsize=65)
    ax.set_ylim(0, 1200)
    ax.set_yticks([0, 150, 300, 450, 600, 750, 900, 1050, 1200])
    ax.tick_params(axis='x', labelsize=64)
    ax.tick_params(axis='y', labelsize=58)
    ax.set_xticks(dimensions)  
    ax.legend(loc='upper center', bbox_to_anchor=(0.5, 1.30), fontsize=50, ncol=3, handletextpad=0.2, columnspacing=1.2, borderpad=0.1)
    ax.yaxis.grid(True, linestyle='-', linewidth=0.8, color='lightgray', zorder=1)


def plot_data_2(ax, data_arrays):
    dimensions = ['2D', '3D', '4D', '5D', '6D']
    labels = ['HalfRing_Pipeline', 'FoldedRing_Pipeline', 'MATE',
              'HalfRing_DimRotation', 'FoldedRing_DimRotation', 'MATEe']
    fill_colors = ['#0582ca', '#ffb703', '#ff87ab', 
                   '#3a6ea5', '#f4a261', '#c77dff']
    line_colors = ['#006494', '#fb8500', '#ff5d8f', 
                   '#004e98', '#e76f51', '#7b2cbf']
    markers = ['o', 'D', '^', 'H', 's', 'v']

    reordered_data_arrays = [data_arrays[i] for i in [0, 2, 4, 1, 3, 5]]

    for i, data in enumerate(reordered_data_arrays):
        ax.plot(dimensions, data, label=labels[i], marker=markers[i], markersize=20, color=line_colors[i], fillstyle='full', markerfacecolor=fill_colors[i], zorder=3, markeredgewidth=2, linewidth=6)

    ax.set_xlabel('Network Dimension', fontsize=60)
    ax.set_ylabel('All-to-All Time (us)', fontsize=65)
    y_max = 250
    interval = 50
    ax.set_yticks(np.arange(0, y_max + interval, interval))
    ax.set_ylim(40, y_max)
    ax.tick_params(axis='x', labelsize=64)
    ax.tick_params(axis='y', labelsize=60)
    ax.yaxis.grid(True, linestyle='-', linewidth=0.8, color='lightgray', zorder=1)

    ax.legend(loc='upper center', bbox_to_anchor=(0.5, 1.29), fontsize=47, ncol=2, handletextpad=0.1, columnspacing=0.5, borderpad=0.1)



if __name__ == "__main__":
    files_1 = [
        '../examples/results/Scalability_exp/Algorithm_weak/No_Fault_Ring/backend_end_to_end.csv',
        '../examples/results/Scalability_exp/Algorithm_weak/No_Fault_HalfRing/backend_end_to_end.csv',
        '../examples/results/Scalability_exp/Algorithm_weak/Single_Fault/backend_end_to_end.csv',
        '../examples/results/Scalability_exp/Algorithm_weak/MATE/backend_end_to_end.csv',
        '../examples/results/Scalability_exp/Algorithm_weak/MATE_Enhanced/backend_end_to_end.csv',
    ]
    files_2 = [
        '../examples/results/Scalability_exp/Scheduling_weak/HalfRing_No_Fault_Pipeline/backend_end_to_end.csv',
        '../examples/results/Scalability_exp/Scheduling_weak/HalfRing_No_Fault_NDTorus/backend_end_to_end.csv',
        '../examples/results/Scalability_exp/Scheduling_weak/Single_Fault_Pipeline/backend_end_to_end.csv',
        '../examples/results/Scalability_exp/Scheduling_weak/Single_Fault_NDTorus/backend_end_to_end.csv',
        '../examples/results/Scalability_exp/Scheduling_weak/MATE/backend_end_to_end.csv',
        '../examples/results/Scalability_exp/Scheduling_weak/MATE_Enhanced/backend_end_to_end.csv',
    ]

    data_arrays_1 = process_input_files_1(files_1)
    data_arrays_2 = process_input_files_2(files_2)

    fig = plt.figure(figsize=(50, 15))
    grid = GridSpec(1, 2, wspace=0.2, hspace=0.45, figure=fig, top=0.81, bottom=0.11)

    ax1 = fig.add_subplot(grid[0])
    plot_data_1(ax1, data_arrays_1)

    ax2 = fig.add_subplot(grid[1])
    plot_data_2(ax2, data_arrays_2)

    plt.savefig('Scalability_Exp.png', dpi=400)
    plt.savefig('Scalability_Exp.pdf')  # Save the figure as one PDF file