# [MICRO 2025] Optimizing All-to-All Collective Communication with Fault Tolerance on Torus Networks

## Directory Structure
- analytical_backend: contains all the source code, experiment scripts, and figure scripts for simulation with **analytical** backend (including implementation of **Ring** algorithm + **Pipeline** scheduling baselines, along with proposed **HalfRing** algorithm, **DimRotation** scheduling, **FoldedRing** algorithm, **MATE/MATEe** scheduling approches)
  - astra-sim: contains all the source code for the simulation (for more information, please see [ASTRA-sim](https://github.com/astra-sim/astra-sim/tree/ASTRA-sim-1.0))
    - workload: parse and orchestrate the workload input file with the Finite State Machine (FSM)
    - system: schedule the workflow of each DNN layer as well as modeling the network topology and collective commuication
      - collective: the implementation of each collective algorithm (including our proposed `HalfRing` & `FoldedRing` algorithm)
      - scheduling: the implementation of inter-dimensional chunk scheduling policies (including our proposed `ND_Torus_Ring` scheduling, i.e., `DimRotation` in the paper)
      - topology: the implementation of common network topologies 
  - build: for the use of compilation
  - examples: all the experiment scripts and results
    - scripts: all the experiment scripts with corresponding directory name
    - results: all the experiment results
  - extern: ASTRA-sim integrated external tools including compute/SCALE-sim, network_backend/analytical, and network_backend/garnet
  - inputs: input comfiguration files for experiments
    - network: all the network input files (system network settings)
    - system: all the system input files (communication implementation)
    - workload: all the workload input files (model description)
  - Pictures: all the figure scripts

- garnet_backend: contains all the source code and experiment scripts for simulation with **GARNET** backend (including implementation of **DORMIN** routing, and **WFR** routing with **SANDWICH** law proposed in [Google TPUv4 routing approach](https://www.usenix.org/conference/nsdi24/presentation/zu))<br>
The directory structure is similar to analytical_backend.

- real_machine: contains the source code and experiment scripts for the implementation and tests on  real machine

## Hardware Requirements
1. For simulation experiments: 
- One machine with Ubuntu 22.04 (Other Ubuntu version should work as well)
2. For real machine tests: 
- Two Huawei Ascend 910B node with PCIe 4.0 interconnection 

## Hardware Requirements
1. For simulation experiments:
- Python + Anaconda
- Astra-SIM with analytical and garnet backend (we have already included both within this repository)
2. For real-machine experiments:
- CANN 8.2.RC1
- torch_npu == 2.1.0.post12

## Simulation Setup

### Overall Workflow
In `analytical_backend/examples/scripts` directory, there are six directories which match Synthetic experiments (Speedup, All-to-All bandwidth, Dimension utilization, i.e., Figure 11, 12, 13), Scalability study (Figure 14), Google Comparison (Figure 15: analytical part), Real model performance (Figure 16), Non-Uniform All-to-All Performance (Figure 17: analytical part), and Multi-fault Performance (Figure 18(b): analytical part), respectively. <br>
In `garnet_backend/examples/scripts` directory, there are three directories which match Google Comparison (Figure 15: garnet part), Non-Uniform All-to-All Performance (Figure 17: garnet part), and Multi-fault Performance (Figure 18(b): garnet part), respectively.<br>
Each directory contains its corresponding shell scripts. The experiment results are stored in `/examples/results` with `csv` format.  After finishing all the experiments, the figure scripts in `analytical_backend/Pictures` use the results to reproduce all the figures.

### Compile the source code (analytical backend)
create a new conda environment
```bash
# create and activate the conda environment
cd analytical_backend/
conda env create -f astra-sim-analytical.yml
conda activate astra-sim-analytical
```
compile astra-sim
```bash
./build/astra_analytical/build.sh -c 
```
### Reproduce the simulation results in the paper (with analytical backend)
reproducing all the experiments may need 9 hours (single core)
```bash
cd examples/scripts/
# Note: run-all.sh will generate log files to correponding experiments, which can be deleted (e.g., Synthetic.log)
bash run-all.sh
```

### Compile the source code (garnet backend)
create a new conda environment
```bash
# create and activate the conda environment
cd garnet_backend/
conda env create -f astra-sim-garnet.yml
conda activate astra-sim-garnet
bash setup_protobuf.sh
```
compile astra-sim (-j8 by default)
```bash
./build/astra_garnet/build.sh -c 
```
### Reproduce the simulation results in the paper (with garnet backend)
reproducing all the experiments could take over two weeks (see multi-core settings in `Run_*.sh`, we **strongly suggest** the users to run three `Run_Google_comp_844_*` scripts with different machines in parallel because each of them could take 4 days)
```bash
cd examples/scripts/
# Note: run-all.sh will generate log files to correponding experiments, which can be deleted (e.g., Google_comp_444.log)
bash run-all.sh
```

## Reproduce the real machine results in the paper
```bash
cd real_machine/
bash Run_All_to_All.sh
```

## Reproduce pictures in the paper
```bash
cd analytical_backend/Pictures
conda activate astra-sim-analytical
bash plot-figure.sh
```