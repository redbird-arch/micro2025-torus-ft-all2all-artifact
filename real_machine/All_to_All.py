import os
from torch_npu.contrib import transfer_to_npu
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.distributed as dist
from torch.nn.parallel import DistributedDataParallel as DDP
from socket import gethostname
import time


rank          = int(os.environ["SLURM_PROCID"])
world_size    = int(os.environ["WORLD_SIZE"])
gpus_per_node = int(os.environ["SLURM_GPUS_ON_NODE"])

assert gpus_per_node == torch.cuda.device_count()
print(f"Hello from rank {rank} of {world_size} on {gethostname()} where there are" \
      f" {gpus_per_node} allocated GPUs per node.", flush=True)

dist.init_process_group("nccl", rank=rank, world_size=world_size)
if rank == 0: print(f"Group initialized? {dist.is_initialized()}", flush=True)

local_rank = rank - gpus_per_node * (rank // gpus_per_node)
torch.cuda.set_device(local_rank)



# 2D-Torus rank
# [[ 0,  1,  2,  3],
#  [ 4,  5,  6,  7],
#  [ 8,  9, 10, 11],
#  [12, 13, 14, 15]]

# Neighbors [left, up, right, down]
Rank_Neighbors = [[4*(i//4)+(i-1+4)%4, (i-4+16)%16, 4*(i//4)+(i+1)%4, (i+4)%16] for i in range(16)]

# print(f'Rank={rank}:Rank_Neighbors={Rank_Neighbors[rank]}')
# Rank=0:Rank_Neighbors=[3, 12, 1, 4]
# Rank=1:Rank_Neighbors=[0, 13, 2, 5]
# Rank=2:Rank_Neighbors=[1, 14, 3, 6]
# Rank=3:Rank_Neighbors=[2, 15, 0, 7]

# Rank=4:Rank_Neighbors=[7, 0, 5, 8]
# Rank=5:Rank_Neighbors=[4, 1, 6, 9]
# Rank=6:Rank_Neighbors=[5, 2, 7, 10]
# Rank=7:Rank_Neighbors=[6, 3, 4, 11]

# Rank=8:Rank_Neighbors=[11, 4, 9, 12]
# Rank=9:Rank_Neighbors=[8, 5, 10, 13]
# Rank=10:Rank_Neighbors=[9, 6, 11, 14]
# Rank=11:Rank_Neighbors=[10, 7, 8, 15]

# Rank=12:Rank_Neighbors=[15, 8, 13, 0]
# Rank=15:Rank_Neighbors=[14, 11, 12, 3]
# Rank=13:Rank_Neighbors=[12, 9, 14, 1]
# Rank=14:Rank_Neighbors=[13, 10, 15, 2]


def Step_XY(input_tensor_list, output_tensor_list):
    # comm [left, up, right, down]
    reqs = []
    
    reqs.append(dist.P2POp(dist.isend, input_tensor_list[0], Rank_Neighbors[rank][0]))
    reqs.append(dist.P2POp(dist.irecv, output_tensor_list[0], Rank_Neighbors[rank][0]))
    reqs.append(dist.P2POp(dist.isend, input_tensor_list[1], Rank_Neighbors[rank][1]))
    reqs.append(dist.P2POp(dist.irecv, output_tensor_list[1], Rank_Neighbors[rank][1]))
    reqs.append(dist.P2POp(dist.isend, input_tensor_list[2], Rank_Neighbors[rank][2]))
    reqs.append(dist.P2POp(dist.irecv, output_tensor_list[2], Rank_Neighbors[rank][2]))
    reqs.append(dist.P2POp(dist.isend, input_tensor_list[3], Rank_Neighbors[rank][3]))
    reqs.append(dist.P2POp(dist.irecv, output_tensor_list[3], Rank_Neighbors[rank][3]))

    reqs = dist.batch_isend_irecv(reqs)

    for req in reqs:
        req.wait()

def Step_X(input_tensor_list, output_tensor_list):
    # comm [left, -, right, -]
    reqs = []

    reqs.append(dist.P2POp(dist.isend, input_tensor_list[0], Rank_Neighbors[rank][0]))
    reqs.append(dist.P2POp(dist.irecv, output_tensor_list[0], Rank_Neighbors[rank][0]))
    reqs.append(dist.P2POp(dist.isend, input_tensor_list[2], Rank_Neighbors[rank][2]))
    reqs.append(dist.P2POp(dist.irecv, output_tensor_list[2], Rank_Neighbors[rank][2]))

    reqs = dist.batch_isend_irecv(reqs)

    for req in reqs:
        req.wait()

def Step_Y(input_tensor_list, output_tensor_list):
    # comm [-, up, -, down]
    reqs = []
 
    reqs.append(dist.P2POp(dist.isend, input_tensor_list[1], Rank_Neighbors[rank][1]))
    reqs.append(dist.P2POp(dist.irecv, output_tensor_list[1], Rank_Neighbors[rank][1]))
    reqs.append(dist.P2POp(dist.isend, input_tensor_list[3], Rank_Neighbors[rank][3]))
    reqs.append(dist.P2POp(dist.irecv, output_tensor_list[3], Rank_Neighbors[rank][3]))

    reqs = dist.batch_isend_irecv(reqs)

    for req in reqs:
        req.wait()
    
    return


def All2All_Baseline(input_tensor_list, output_tensor_list):
    # phase 1 (X), step_num = 6
    for i in range(6):
        Step_X(input_tensor_list, output_tensor_list)

    # phase 2 (XY), step_num = 6
    for i in range(6):
        Step_XY(input_tensor_list, output_tensor_list)
    
    # phase 3 (XY), step_num = 6
    for i in range(6):
        Step_XY(input_tensor_list, output_tensor_list)

    # phase 4 (Y), step_num = 6
    for i in range(6):
        Step_Y(input_tensor_list, output_tensor_list)

    return 


def All2All_HalfRing(input_tensor_list, output_tensor_list):
    # phase 1 (XY), step_num = 4
    for i in range(4):
        Step_XY(input_tensor_list, output_tensor_list)

    # phase 2 (XY), step_num = 4
    for i in range(4):
        Step_XY(input_tensor_list, output_tensor_list)

    return 

# 0-1 link fault
def Step_XY_Fault(input_tensor_list, output_tensor_list):
    # comm [left, up, right, down]
    reqs = []
    
    #  0-1-2-3 X fault, only Y
    if rank in [0,1,2,3]:
        reqs.append(dist.P2POp(dist.isend, input_tensor_list[1], Rank_Neighbors[rank][1]))
        reqs.append(dist.P2POp(dist.irecv, output_tensor_list[1], Rank_Neighbors[rank][1]))
        reqs.append(dist.P2POp(dist.isend, input_tensor_list[3], Rank_Neighbors[rank][3]))
        reqs.append(dist.P2POp(dist.irecv, output_tensor_list[3], Rank_Neighbors[rank][3]))
    else:
        reqs.append(dist.P2POp(dist.isend, input_tensor_list[0], Rank_Neighbors[rank][0]))
        reqs.append(dist.P2POp(dist.irecv, output_tensor_list[0], Rank_Neighbors[rank][0]))
        reqs.append(dist.P2POp(dist.isend, input_tensor_list[1], Rank_Neighbors[rank][1]))
        reqs.append(dist.P2POp(dist.irecv, output_tensor_list[1], Rank_Neighbors[rank][1]))
        reqs.append(dist.P2POp(dist.isend, input_tensor_list[2], Rank_Neighbors[rank][2]))
        reqs.append(dist.P2POp(dist.irecv, output_tensor_list[2], Rank_Neighbors[rank][2]))
        reqs.append(dist.P2POp(dist.isend, input_tensor_list[3], Rank_Neighbors[rank][3]))
        reqs.append(dist.P2POp(dist.irecv, output_tensor_list[3], Rank_Neighbors[rank][3]))
        
    reqs = dist.batch_isend_irecv(reqs)

    for req in reqs:
        req.wait()

def Three_Hop_Pipeline(input_tensor, output_tensor, start_id, medium_1_id, medium_2_id, end_id):
    input_chunk_list = list(input_tensor.chunk(3))
    output_chunk_list = list(output_tensor.chunk(3))

    # pp 1
    pp_1_reqs = []
    if rank == start_id:
        pp_1_reqs.append(dist.P2POp(dist.isend, input_chunk_list[0], medium_1_id))
    elif rank == medium_1_id:
        pp_1_reqs.append(dist.P2POp(dist.irecv, output_chunk_list[0], start_id))

    if pp_1_reqs:
        pp_1_reqs = dist.batch_isend_irecv(pp_1_reqs)
        for req in pp_1_reqs:
            req.wait()
    
    # pp2
    pp_2_reqs = []
    if rank == start_id:
        pp_2_reqs.append(dist.P2POp(dist.isend, input_chunk_list[1], medium_1_id))
    elif rank == medium_1_id:
        pp_2_reqs.append(dist.P2POp(dist.isend, output_chunk_list[0], medium_2_id))
        pp_2_reqs.append(dist.P2POp(dist.irecv, output_chunk_list[1], start_id))
    elif rank == medium_2_id:
        pp_2_reqs.append(dist.P2POp(dist.irecv, output_chunk_list[0], medium_1_id))

    if pp_2_reqs:
        pp_2_reqs = dist.batch_isend_irecv(pp_2_reqs)
        for req in pp_2_reqs:
            req.wait()

    # pp3
    pp_3_reqs = []
    if rank == start_id:
        pp_3_reqs.append(dist.P2POp(dist.isend, input_chunk_list[2], medium_1_id))
    elif rank == medium_1_id:
        pp_3_reqs.append(dist.P2POp(dist.isend, output_chunk_list[1], medium_2_id))
        pp_3_reqs.append(dist.P2POp(dist.irecv, output_chunk_list[2], start_id))
    elif rank == medium_2_id:
        pp_3_reqs.append(dist.P2POp(dist.isend, output_chunk_list[0], end_id))
        pp_3_reqs.append(dist.P2POp(dist.irecv, output_chunk_list[1], medium_1_id))
    elif rank == end_id:
        pp_3_reqs.append(dist.P2POp(dist.irecv, output_chunk_list[0], medium_2_id))

    if pp_3_reqs:
        pp_3_reqs = dist.batch_isend_irecv(pp_3_reqs)
        for req in pp_3_reqs:
            req.wait()

    # pp4
    pp_4_reqs = []
    if rank == medium_1_id:
        pp_4_reqs.append(dist.P2POp(dist.isend, output_chunk_list[2], medium_2_id))
    elif rank == medium_2_id:
        pp_4_reqs.append(dist.P2POp(dist.isend, output_chunk_list[1], end_id))
        pp_4_reqs.append(dist.P2POp(dist.irecv, output_chunk_list[2], medium_1_id))
    elif rank == end_id:
        pp_4_reqs.append(dist.P2POp(dist.irecv, output_chunk_list[1], medium_2_id))

    if pp_4_reqs:
        pp_4_reqs = dist.batch_isend_irecv(pp_4_reqs)
        for req in pp_4_reqs:
            req.wait()

    # pp5
    pp_5_reqs = []
    if rank == medium_2_id:
        pp_5_reqs.append(dist.P2POp(dist.isend, output_chunk_list[2], end_id))
    elif rank == end_id:
        pp_5_reqs.append(dist.P2POp(dist.irecv, output_chunk_list[2], medium_2_id))

    if pp_5_reqs:
        pp_5_reqs = dist.batch_isend_irecv(pp_5_reqs)
        for req in pp_5_reqs:
            req.wait()

    return


def Step_MATE(input_tensor_list, output_tensor_list, output_tensor_list_2):
    # comm [left, up, right, down]
    reqs_rest_links = []

    for step_i in range(6):
        # link [0-1-2-3] (unidirection)
        if rank == 0:
            reqs_rest_links.append(dist.P2POp(dist.irecv, output_tensor_list_2[0][0:int(input_tensor_list[0].size(0)/4),:], 3))
        elif rank == 1:
            reqs_rest_links.append(dist.P2POp(dist.isend, input_tensor_list[0][0:int(input_tensor_list[0].size(0)/4),:], 2))
        elif rank == 2:
            reqs_rest_links.append(dist.P2POp(dist.isend, input_tensor_list[0][0:int(input_tensor_list[0].size(0)/4),:], 3))
            reqs_rest_links.append(dist.P2POp(dist.irecv, output_tensor_list_2[1][0:int(input_tensor_list[0].size(0)/4),:], 1))
        elif rank == 3:
            reqs_rest_links.append(dist.P2POp(dist.isend, input_tensor_list[0][0:int(input_tensor_list[0].size(0)/4),:], 0))
            reqs_rest_links.append(dist.P2POp(dist.irecv, output_tensor_list_2[2][0:int(input_tensor_list[0].size(0)/4),:], 2))

    for step_i in range(4): 
        # rest link (bidirection)
        Three_Hop_Pipeline(input_tensor_list[0][:int(3*input_tensor_list[0].size(0)/8),:], output_tensor_list[0][:int(3*input_tensor_list[0].size(0)/8),:], 0, 4, 5, 1)
        Three_Hop_Pipeline(input_tensor_list[0][:int(3*input_tensor_list[0].size(0)/8),:], output_tensor_list[0][:int(3*input_tensor_list[0].size(0)/8),:], 1, 13, 12, 0)

        Three_Hop_Pipeline(input_tensor_list[1][:int(3*input_tensor_list[1].size(0)/8),:], output_tensor_list[1][:int(3*input_tensor_list[1].size(0)/8),:], 1, 5, 6, 2)
        Three_Hop_Pipeline(input_tensor_list[1][:int(3*input_tensor_list[1].size(0)/8),:], output_tensor_list[1][:int(3*input_tensor_list[1].size(0)/8),:], 2, 14, 13, 1)

        Three_Hop_Pipeline(input_tensor_list[2][:int(3*input_tensor_list[2].size(0)/8),:], output_tensor_list[2][:int(3*input_tensor_list[2].size(0)/8),:], 2, 6, 7, 3)
        Three_Hop_Pipeline(input_tensor_list[2][:int(3*input_tensor_list[2].size(0)/8),:], output_tensor_list[2][:int(3*input_tensor_list[2].size(0)/8),:], 3, 15, 14, 2)

        Three_Hop_Pipeline(input_tensor_list[3][:int(3*input_tensor_list[3].size(0)/8),:], output_tensor_list[3][:int(3*input_tensor_list[3].size(0)/8),:], 3, 7, 4, 0)
        Three_Hop_Pipeline(input_tensor_list[3][:int(3*input_tensor_list[3].size(0)/8),:], output_tensor_list[3][:int(3*input_tensor_list[3].size(0)/8),:], 0, 12, 15, 3)

    
    for step_i in range(6):
        # link [0-1-2-3] (unidirection)
        Three_Hop_Pipeline(input_tensor_list[0][0:int(input_tensor_list[0].size(0)/4),:], output_tensor_list_2[3][0:int(input_tensor_list[0].size(0)/4),:], 0, 3, 2, 1)

    if reqs_rest_links:
        reqs_rest_links = dist.batch_isend_irecv(reqs_rest_links)
        for req in reqs_rest_links:
            req.wait()


# 0-1 link fault
def All2All_MATE(input_tensor_list, output_tensor_list, output_tensor_list_2):
    # phase 1 (XY), step_num = 4
    for i in range(4):
        Step_XY_Fault(input_tensor_list, output_tensor_list)

    # phase 1 MATE
    Step_MATE(input_tensor_list, output_tensor_list, output_tensor_list_2)

    # phase 2 (XY), step_num = 4
    for i in range(4):
        Step_XY_Fault(input_tensor_list, output_tensor_list)

    # phase 2 MATE
    Step_MATE(input_tensor_list, output_tensor_list, output_tensor_list_2)
        
    return 



tensor_size_A_list = [11520] 
tensor_size_B_list = [512, 1024, 2048, 4096, 8192, 16484, 32768, 65536, 131072] # [128,512,1024,2048,4096]

repeat_test_num = 100

for tensor_size_A in tensor_size_A_list:
    for tensor_size_B in tensor_size_B_list:
        if dist.get_rank()== 0:
            print(f'------------Comm-Tensor-Size=[{tensor_size_A},{tensor_size_B}]----------')


        # -----------------------------------All2All_Baseline-----------------------------------
        average_time = 0
        input_tensor_list = [torch.rand(int(tensor_size_A/24), tensor_size_B).cuda() for _ in range(4)]
        output_tensor_list = [torch.empty_like(input_tensor_list[0]) for _ in range(4)]
        
        # init
        All2All_Baseline(input_tensor_list, output_tensor_list)

        for i in range(repeat_test_num):
        
            dist.barrier()
            # torch.cuda.synchronize()
            start = time.time()
            
            All2All_Baseline(input_tensor_list, output_tensor_list)

            dist.barrier()
            # torch.cuda.synchronize()
            single_time = time.time()-start
            if dist.get_rank()== 0:
                print(f'[All2All_Baseline Time] {single_time}')
            average_time += single_time

        if dist.get_rank()== 0:
            print(f'[All2All_Baseline Average Time] {average_time/repeat_test_num}')

        
        # -----------------------------------All2All_HalfRing-----------------------------------
        average_time = 0
        input_tensor_list = [torch.rand(int(tensor_size_A/16), tensor_size_B).cuda() for _ in range(4)]
        output_tensor_list = [torch.empty_like(input_tensor_list[0]) for _ in range(4)]
        output_tensor_list_2 = [torch.empty_like(input_tensor_list[0]) for _ in range(4)]

        # init
        All2All_HalfRing(input_tensor_list, output_tensor_list)

        for i in range(repeat_test_num):
        
            dist.barrier()
            # torch.cuda.synchronize()
            start = time.time()
            
            All2All_HalfRing(input_tensor_list, output_tensor_list)

            dist.barrier()
            # torch.cuda.synchronize()
            single_time = time.time()-start
            if dist.get_rank()== 0:
                print(f'[All2All_HalfRing Time] {single_time}')
            average_time += single_time

        if dist.get_rank()== 0:
            print(f'[All2All_HalfRing Average Time] {average_time/repeat_test_num}')



        # -----------------------------------All2All_MATE-----------------------------------
        average_time = 0

        #init
        All2All_MATE(input_tensor_list, output_tensor_list, output_tensor_list_2)

        for i in range(repeat_test_num):
        
            dist.barrier()
            # torch.cuda.synchronize()
            start = time.time()
            
            All2All_MATE(input_tensor_list, output_tensor_list, output_tensor_list_2)

            dist.barrier()
            # torch.cuda.synchronize()
            single_time = time.time()-start
            if dist.get_rank()== 0:
                print(f'[All2All_MATE Time] {single_time}')
            average_time += single_time

        if dist.get_rank()== 0:
            print(f'[All2All_MATE Average Time] {average_time/repeat_test_num}')

dist.destroy_process_group()