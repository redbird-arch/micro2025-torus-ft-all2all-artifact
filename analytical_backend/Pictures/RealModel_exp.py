import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection
import numpy as np
from matplotlib.lines import Line2D

# Set the root directory
root_dir = "../examples/results/Real_Model"

# Define the methods and datasets
methods = ["No_Fault_Ring", "No_Fault_HalfRing", "Single_Fault", "MATE", "MATE_Enhanced"]
datasets = [
    "DLRM_TPUv3_Inference_TPUv3",
    "Wide&Deep_TPUv3_Inference_TPUv3",
    "Deepspeed-1.3B+MoE-128_DP+EP+TP8_TPUv3_Inference_TPUv3",
    "Mistral7Bx8_DP+EP+TP8_TPUv3_Inference_TPUv3",
    "DLRM_TPUv3_Training_TPUv3",
    "Wide&Deep_TPUv3_Training_TPUv3",
    "Deepspeed-1.3B+MoE-128_DP+EP+TP8_TPUv3_Training_TPUv3",
    "Mistral7Bx8_DP+EP+TP8_TPUv3_Training_TPUv3",
    "DLRM_TPUv4_Inference_TPUv4",
    "Wide&Deep_TPUv4_Inference_TPUv4",
    "Deepspeed-1.3B+MoE-128_DP+EP+TP8_TPUv4_Inference_TPUv4",
    "Mistral7Bx8_DP+EP+TP8_TPUv4_Inference_TPUv4",
    "DLRM_TPUv4_Training_TPUv4",
    "Wide&Deep_TPUv4_Training_TPUv4",
    "Deepspeed-1.3B+MoE-128_DP+EP+TP8_TPUv4_Training_TPUv4",
    "Mistral7Bx8_DP+EP+TP8_TPUv4_Training_TPUv4"
]

dataset_labels = ["DLRM", "Wide&Deep", "DeepSpeedMoE", "Mixtral"]
method_labels = ["Ring+PL", "HalfR+DR", "FoldR+PL", "MATE", "MATEe"]
# Initialize a dictionary to store data
data = {dataset: {method: {'Exposed_AllToAll': [], 'Exposed_AllReduce': [], 'Overlap': [], 'Computation': [], 'TotalTime': []} for method in methods} for dataset in datasets}

# Function to construct file path based on method and dataset
def construct_file_path(method, dataset, file_name):
    suffix = "NoFault" if method in ["No_Fault_Ring", "No_Fault_HalfRing"] else "FaultTolerance"
    folder_name = f"{dataset}_{suffix}"
    return os.path.join(root_dir, method, folder_name, file_name)

# Read and compute custom runtime breakdown
for method in methods:
    for dataset in datasets:
        backend_file_path = construct_file_path(method, dataset, "backend_end_to_end.csv")
        end_to_end_file_path = construct_file_path(method, dataset, "EndToEnd.csv")

        if os.path.exists(backend_file_path) and os.path.exists(end_to_end_file_path):
            df_backend = pd.read_csv(backend_file_path)
            df_end_to_end = pd.read_csv(end_to_end_file_path).dropna(how='all')

            compute_time = df_backend['ComputeTime'].iloc[0]
            total_comm = df_backend['CommsTime'].iloc[0]
            exposed_comm = df_backend['ExposedCommsTime'].iloc[0]

            is_moe = 'Deepspeed' in dataset or 'Mistral' in dataset
            is_dlrm = 'DLRM' in dataset or 'Wide' in dataset

            # Calculate exposed_AllReduce
            if is_moe:
                # Sum of specific rows from fwd and ig columns
                fwd_rows = df_end_to_end[df_end_to_end['Unnamed: 0'].isin(['Cprj', 'MLPOut'])]
                ig_rows = df_end_to_end[df_end_to_end['Unnamed: 0'].isin(['Q', 'Feedforwardmlp1'])]

                exposed_allreduce = 0
                if 'fwd exposed comm' in df_end_to_end.columns:
                    exposed_allreduce += fwd_rows['fwd exposed comm'].sum()
                if 'ig exposed comm' in df_end_to_end.columns:
                    exposed_allreduce += ig_rows['ig exposed comm'].sum()

            elif is_dlrm:
                non_embedding_rows = df_end_to_end[df_end_to_end['Unnamed: 0'] != 'Embedding']
                linear_row = df_end_to_end[df_end_to_end['Unnamed: 0'] == 'Linear_Bottom1']

                exposed_allreduce = 0
                if 'fwd exposed comm' in df_end_to_end.columns:
                    exposed_allreduce += non_embedding_rows['fwd exposed comm'].sum()
                if 'wg exposed comm' in df_end_to_end.columns:
                    exposed_allreduce += non_embedding_rows['wg exposed comm'].sum()
                if 'ig exposed comm' in df_end_to_end.columns:
                    exposed_allreduce += non_embedding_rows['ig exposed comm'].sum()
                if 'wg total comm' in df_end_to_end.columns:
                    exposed_allreduce += linear_row['wg total comm'].sum()
            else:
                exposed_allreduce = 0

            # exposed_AllToAll = ExposedCommsTime - exposed_AllReduce
            exposed_alltoall = exposed_comm - exposed_allreduce

            is_inference = 'Inference' in dataset
            if is_inference:
                if is_dlrm:
                    embedding_row = df_end_to_end[df_end_to_end['Unnamed: 0'] == 'Embedding']
                    # Calculate overlap using the additional columns in the "Embedding" row
                    additional_overlap = 0
                    if not embedding_row.empty:
                        additional_overlap += embedding_row['fwd total comm'].sum()
                        additional_overlap -= embedding_row['fwd exposed comm'].sum()
                    # Add the computed overlap to the previous overlap values
                    comp_comm_overlap = additional_overlap
                elif is_moe:    
                    comp_comm_overlap = 0
                computation = compute_time - comp_comm_overlap
            else:
                # Training: calculate overlap
                if is_dlrm:
                    # Get all rows excluding 'Linear_Bottom1'
                    overlap_rows = df_end_to_end[df_end_to_end['Unnamed: 0'] != 'Linear_Bottom1']
                    # Get the "Embedding" row for the specific comms
                    embedding_row = df_end_to_end[df_end_to_end['Unnamed: 0'] == 'Embedding']
                    # Calculate overlap using the additional columns in the "Embedding" row
                    additional_overlap = 0
                    if not embedding_row.empty:
                        additional_overlap += embedding_row['fwd total comm'].sum()
                        additional_overlap += embedding_row['ig total comm'].sum()
                        additional_overlap -= embedding_row['fwd exposed comm'].sum()
                        additional_overlap -= embedding_row['ig exposed comm'].sum()
                    # Add the computed overlap to the previous overlap values
                    comp_comm_overlap = overlap_rows['wg total comm'].sum() + additional_overlap
                elif is_moe:
                    comp_comm_overlap = df_end_to_end['wg total comm'].sum()
                else:
                    comp_comm_overlap = 0
                computation = compute_time - comp_comm_overlap
            
            # Store each component
            data[dataset][method]['Exposed_AllToAll'] = [exposed_alltoall]
            data[dataset][method]['Exposed_AllReduce'] = [exposed_allreduce]
            data[dataset][method]['Overlap'] = [comp_comm_overlap]
            data[dataset][method]['Computation'] = [computation]
            data[dataset][method]['TotalTime'].append(df_backend['CommsTime'].iloc[0])

        else:
            print(f"Missing file for {method} - {dataset}")


# Calculate and print speedups for each dataset
speedup_totals = {method: {'TotalTime': [], 'Exposed_AllToAll': []} for method in methods if method != "No_Fault_Ring"}

for dataset in datasets:
    base_total_time = data[dataset]['No_Fault_Ring']['TotalTime'][0] if data[dataset]['No_Fault_Ring']['TotalTime'] else float('inf')
    base_exposed_time = data[dataset]['No_Fault_Ring']['Exposed_AllToAll'][0] if data[dataset]['No_Fault_Ring']['Exposed_AllToAll'] else float('inf')
    
    print(f"\nSpeedups for {dataset}:")
    for method in methods:
        if method != "No_Fault_Ring":
            method_total_time = data[dataset][method]['TotalTime'][0] if data[dataset][method]['TotalTime'] else float('inf')
            method_exposed_time = data[dataset][method]['Exposed_AllToAll'][0] if data[dataset][method]['Exposed_AllToAll'] else float('inf')
            total_time_speedup = base_total_time / method_total_time if method_total_time else float('inf')
            exposed_time_speedup = base_exposed_time / method_exposed_time if method_exposed_time else float('inf')
            print(f"  {method} - Total Time Speedup: {total_time_speedup:.2f}, Exposed Comms Time Speedup: {exposed_time_speedup:.2f}")
            speedup_totals[method]['TotalTime'].append(total_time_speedup)
            speedup_totals[method]['Exposed_AllToAll'].append(exposed_time_speedup)

# Calculate and print average speedups across all datasets
print("\nAverage Speedups across all datasets:")
for method in methods:
    if method != "No_Fault_Ring":
        avg_total_time_speedup = np.mean(speedup_totals[method]['TotalTime'])
        avg_exposed_time_speedup = np.mean(speedup_totals[method]['Exposed_AllToAll'])
        print(f"{method} - Average Total Time Speedup: {avg_total_time_speedup:.2f}, Average Exposed Comms Time Speedup: {avg_exposed_time_speedup:.2f}")

# Normalize data
for dataset in datasets:
    normalization_factor = data[dataset]["No_Fault_Ring"]['TotalTime'][0]
    for method in methods:
        for key in data[dataset][method]:
            if data[dataset][method][key]:
                data[dataset][method][key] = [round(x / normalization_factor, 2) for x in data[dataset][method][key]]
# Normalize data for ExposedCommsTime to calculate speedups
for dataset in datasets:
    normalization_factor = data[dataset]["No_Fault_Ring"]['Exposed_AllToAll'][0]
    for method in methods:
        data[dataset][method]['Speedup'] = [normalization_factor / x if x != 0 else 0 for x in data[dataset][method]['Exposed_AllToAll']]

# Define the plot
fig, ax1 = plt.subplots(figsize=(22.5, 6))
bar_width = 0.1
group_spacing = 0.02
startoffset = 0.02
colors = ['#eca0ff', '#ffb703', '#aab2ff', '#84ffc9']  # AllToAll, AllReduce, Overlap, Computation

group_names = ["Inference on 8x8 TPUv3", "Training on 8x8 TPUv3", "Inference on 8x8x8 TPUv4", "Training on 8x8x8 TPUv4"]
ax2 = ax1.twinx()

for i, dataset in enumerate(datasets):
    base_index = i * (len(methods) * (bar_width + group_spacing) + startoffset)
    is_training = "Training" in dataset
    speedup_points = []
    positions = []
    for j, method in enumerate(methods):
        index = base_index + j * (bar_width + group_spacing)
        exposed_alltoall = data[dataset][method]['Exposed_AllToAll'][0]
        exposed_allreduce = data[dataset][method]['Exposed_AllReduce'][0]
        comp_comm_overlap = data[dataset][method]['Overlap'][0]
        # change the computation to a abs value
        # computation = abs(data[dataset][method]['Computation'][0])
        computation = data[dataset][method]['Computation'][0]

        speedup = data[dataset][method]['Speedup'][0]
        # 4-layer bar for training
        ax1.bar(index, exposed_alltoall, bar_width, color=colors[0], zorder=3, edgecolor='black', label='All-to-All' if i == 0 and j == 0 else "")
        ax1.bar(index, exposed_allreduce, bar_width, bottom=exposed_alltoall, color=colors[1], zorder=3, edgecolor='black', label='All-Reduce' if i == 0 and j == 0 else "")
        ax1.bar(index, comp_comm_overlap, bar_width, bottom=exposed_alltoall + exposed_allreduce, color=colors[2], zorder=3, edgecolor='black', label='Computation-Communication-Overlap' if i == 0 and j == 0 else "")
        ax1.bar(index, computation, bar_width, bottom=exposed_alltoall + exposed_allreduce + comp_comm_overlap, color=colors[3], zorder=3, edgecolor='black', label='Computation' if i == 0 and j == 0 else "")


        speedup_points.append(speedup)
        positions.append(index)

        # Method labels
        ax1.text(index, -0.01, method_labels[j], ha='center', va='top', rotation=90, fontsize = 13)


        # Draw vertical lines between datasets
        if j == len(methods) - 1 and i < len(datasets) - 1:  # Check if it's the last method of the current dataset
            length = -0.55
            if (i % 4 == 3): 
                length = -0.8
            next_base_index = (i + 1) * (len(methods) * (bar_width + 0.02) + startoffset)
            next_index = next_base_index
            mid_point = (index + next_index) / 2
            ax1.annotate('', xy=(mid_point, 0), xytext=(mid_point, length),
                xycoords='data', textcoords='data',
                arrowprops=dict(arrowstyle='-', color='black', linestyle='--', lw=1))

        length = -0.8    
        first_index = 0 * (len(methods) * (bar_width + group_spacing) + startoffset) - (group_spacing / 2)
        ax1.annotate('', xy=(first_index - 0.05, 0), xytext=(first_index - 0.05, length),
            xycoords='data', textcoords='data',
            arrowprops=dict(arrowstyle='-', color='black', linestyle='--', lw=1))
        last_index = (len(datasets) - 1) * (len(methods) * (bar_width + group_spacing) + startoffset) + (len(methods) * bar_width)
        ax1.annotate('', xy=(last_index + 0.05, 0), xytext=(last_index + 0.05, length),
            xycoords='data', textcoords='data',
            arrowprops=dict(arrowstyle='-', color='black', linestyle='--', lw=1))
        
    # Plotting the speedup line for each dataset
    ax2.plot(positions, speedup_points, 'o-', color='black', zorder = 5, label="All-to-All Speedup" if i == 0 else "")
    if (i + 1) % 4 == 0:  
        group_label_index = base_index - 5 * (bar_width + group_spacing)
        group_name_index = (i // 4)
        ax1.text(group_label_index, -1.1, group_names[group_name_index],
                ha='center', va='top', fontsize=21, color='black')
    # Dataset labels
    
    depth = -0.88
    label_size = 13.1
    if (i % 4 == 0):
        ax1.text(base_index + (len(methods) * bar_width / 2),depth, dataset_labels[0], ha='center', va='top', fontsize = label_size)
    elif (i % 4 == 1):
        ax1.text(base_index + (len(methods) * bar_width / 2), depth, dataset_labels[1], ha='center', va='top', fontsize = label_size)
    elif (i % 4 == 2):
        ax1.text(base_index + (len(methods) * bar_width / 2), depth, dataset_labels[2], ha='center', va='top', fontsize = label_size)
    else:
        ax1.text(base_index + (len(methods) * bar_width / 2), depth, dataset_labels[3], ha='center', va='top', fontsize = label_size)
        
first_bar_position = 0 * (len(methods) * (bar_width + group_spacing) + startoffset)
last_bar_position = (len(datasets) - 1) * (len(methods) * (bar_width + group_spacing) + startoffset) + (len(methods) * bar_width)
ax1.set_xlim(first_bar_position - bar_width, last_bar_position + bar_width)



# Adjust plot
ax1.set_ylim(0.0, 2.48)  
ax1.set_yticks([0, 0.5, 1.0, 1.5, 2.0, 2.5]) 
ax1.tick_params(axis='y', labelsize=20)
ax2.set_ylim(0.0, 2.48)  
ax2.set_yticks([0, 0.5, 1.0, 1.5, 2.0, 2.5]) 
ax2.tick_params(axis='y', labelsize=20)

# Get handles and labels for ax1
custom_line = Line2D([0], [0], color='black', marker='o', markersize=5, linestyle='-', linewidth=2)
handles1, labels1 = ax1.get_legend_handles_labels()
handles1.append(custom_line)
labels1.append("All-to-All Speedup")     
ax1.legend(handles1, labels1, loc='upper center', bbox_to_anchor=(0.5, 1.165), ncol=5, fontsize=20, columnspacing=2.8, handletextpad=0.6, borderpad=0.2)
ax1.set_ylabel("Normalized time Breakdown", fontsize = 19)
ax2.set_ylabel("All-to-All Speedup", fontsize = 20)
ax1.yaxis.grid(True, linestyle='-', linewidth=0.5, color='lightgray', zorder=1)
plt.xticks([])  # Remove x-axis tick labels
plt.tight_layout()
plt.savefig('RealModel_Exp.png', dpi=400)
plt.savefig('RealModel_Exp.pdf')