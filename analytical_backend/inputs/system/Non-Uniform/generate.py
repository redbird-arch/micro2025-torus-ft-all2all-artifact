# generate_txts.py

template = """scheduling-policy: LIFO
endpoint-delay: 1
active-chunks-per-dimension: 1
preferred-dataset-splits: 3
boost-mode: 0
all-reduce-implementation: ring_ring_ring
all-gather-implementation: ring_ring_ring
reduce-scatter-implementation: ring_ring_ring
all-to-all-implementation: halfring_halfring_halfring
collective-optimization: localBWAware
intra-dimension-scheduling: SCF
inter-dimension-scheduling: ND_Torus_Ring_AlltoAll_AllReduce
link-failure-per-dimension: 0_0_0
link-failure-scheduling: mate_enhanced
non_uniform: {}
"""

for i in range(0, 33):
    filename = f"{i}.txt"
    with open(filename, "w") as f:
        f.write(template.format(i))

print("Successfully generated 33 txt files.")