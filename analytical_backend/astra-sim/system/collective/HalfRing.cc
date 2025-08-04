/*
# File name  :    HalfRing.cc
# Author     :    Galois
# Time       :    2024/05/11 16:42:52
*/

#include "HalfRing.hh"
#include "astra-sim/system/PacketBundle.hh"
#include "astra-sim/system/RecvPacketEventHadndlerData.hh"
namespace AstraSim {
HalfRing::HalfRing(
    ComType type,
    int id,
    int layer_num,
    RingTopology* ring_topology,
    uint64_t data_size,
    RingTopology::Direction direction,
    InjectionPolicy injection_policy,
    bool boost_mode,
    int link_failure_in_this_dimension,
    LinkFailureScheduling link_failure_scheduling,
    int num_dimensions,
    int chunk_stage,
    int nodes_num_of_failed_ring,
    std::vector<int> physical_dims,
    int failure_type,
    int non_uniform_flag) 
    : Algorithm(layer_num) {
  // std::cout<<"Ring checkmark 0"<<std::endl;
  this->comType = type;
  this->id = id;
  this->logicalTopology = ring_topology;
  this->data_size = data_size;
  this->orig_data_size = data_size;
  this->local_link_failure_scheduling = link_failure_scheduling;
  this->local_nodes_num_of_failed_ring = nodes_num_of_failed_ring;
  this->local_link_failure_in_this_dimension = link_failure_in_this_dimension;
  this->direction = direction;
  this->nodes_in_ring = ring_topology->get_nodes_in_ring();
  this->current_receiver = ring_topology->get_receiver_node(id, direction);
  this->current_sender = ring_topology->get_sender_node(id, direction);
  this->parallel_reduce = 1;
  this->injection_policy = injection_policy;
  this->total_packets_sent = 0;
  this->total_packets_received = 0;
  this->free_packets = 0;
  this->zero_latency_packets = 0;
  this->non_zero_latency_packets = 0;
  this->toggle = false;
  this->name = Name::HalfRing;
  this->enabled = true;
  this->local_chunk_stage = chunk_stage;
  this->local_failure_type = failure_type; 
  this->num_dimensions = num_dimensions;
  this->max_physical_dim_value = 0;
  this->non_uniform_flag = non_uniform_flag; 

  // Galois change: enable the non-uniform All-to-All
  if (non_uniform_flag != 0) {
    // Galois note: it's hard-coded
    data_size = GetMoEDistributionValue("../../inputs/workload/Non-Uniform-MoE/MoE_Distribution.txt", non_uniform_flag-1, (id % 8)) * data_size / 8192; // Galois change: get the MoE distribution value from external txt file
  }
  assert(!physical_dims.empty()); 
  this->max_physical_dim_value = *std::max_element(physical_dims.begin(), physical_dims.end()); 
  assert(this->max_physical_dim_value != 0);

  if (boost_mode) {
    this->enabled = ring_topology->is_enabled();
  }
  if (ring_topology->dimension == RingTopology::Dimension::Local) {
    transmition = MemBus::Transmition::Fast;
  } else {
    transmition = MemBus::Transmition::Usual;
  }

  assert (type == ComType::All_to_All); // HalfRing is only for All-to-All collective

  if (link_failure_in_this_dimension == 0){
    // Half Ring Algorithm without any failure
    if (nodes_in_ring == 2){
        this->stream_count = 1 * 2;
    } else {
        if (nodes_in_ring % 2 == 0) {
          this->stream_count = ceil(nodes_in_ring * nodes_in_ring * 2 / 8);  
        } else {
          this->stream_count = (nodes_in_ring * nodes_in_ring - 1) * 2 / 8;
        }
    }
  } else if (link_failure_in_this_dimension == 1) {
    // Fault Tolerance Baseline Algorithm FoldedRing
    if (nodes_in_ring == 2) {
        std::cout << "Link failure happens in a 2-node ring!" << std::endl;
        assert (nodes_in_ring != 2); 
    } else {
        this->stream_count = (nodes_in_ring - 1) * nodes_in_ring / 2; 
    }
  } else {
    std::cout << "Too many link failures in this ring!" << std::endl;
    assert (link_failure_in_this_dimension == 1); 
  }
  
  if ((link_failure_scheduling == LinkFailureScheduling::Mate || 
       link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced) &&
      ((chunk_stage % 2 == 1 && failure_type != 1 && failure_type != 4) ||
       (chunk_stage % 3 != 0 && (failure_type == 1 || failure_type == 4)))) {
    if (nodes_num_of_failed_ring % 2 == 0) {
      this->stream_count = ceil(static_cast<double>(nodes_num_of_failed_ring * nodes_num_of_failed_ring) / 8 * 2);  
    } else {
      this->stream_count = (nodes_num_of_failed_ring * nodes_num_of_failed_ring - 1) / 8 * 2;
    } 
  } 
  if (link_failure_scheduling == LinkFailureScheduling::Mate &&
      ((chunk_stage % 2 == 0 && failure_type != 1 && failure_type != 4) ||
       (chunk_stage % 3 == 0 && (failure_type == 1 || failure_type == 4)))) {
    if (nodes_num_of_failed_ring == 2){
        this->stream_count = 1 * 2;
    } else {
        if (nodes_num_of_failed_ring % 2 == 0) {
        this->stream_count = ceil(static_cast<double>(max_physical_dim_value * max_physical_dim_value) / 8 * 2); 
        } else {
        this->stream_count = (max_physical_dim_value * max_physical_dim_value - 1) / 8 * 2;
        }
    } 
  } 
  if (link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced &&
      ((chunk_stage % 2 == 0 && failure_type != 1 && failure_type != 4) ||
       (chunk_stage % 3 == 0 && (failure_type == 1 || failure_type == 4))) &&
      link_failure_in_this_dimension == 0) {
    if (nodes_num_of_failed_ring == 2){
        this->stream_count = 1 * 2;
    } else {
        if (nodes_num_of_failed_ring % 2 == 0) {
        this->stream_count = ceil(static_cast<double>(max_physical_dim_value * max_physical_dim_value) / 8 * 2); 
        } else {
        this->stream_count = (max_physical_dim_value * max_physical_dim_value - 1) / 8 * 2;
        }
    } 
  } 
  
  this->total_stream_count = this->stream_count; 
  switch (injection_policy) {
  case InjectionPolicy::Aggressive:
      this->parallel_reduce = nodes_in_ring - 1;
      break;
  case InjectionPolicy::Normal:
      this->parallel_reduce = 1;
      break;
  default:
      this->parallel_reduce = 1;
      break;
  }

  if (type == ComType::All_to_All || type == ComType::All_Gather) {
    max_count = 0;
  } else {
    max_count = nodes_in_ring - 1;
  }
  remained_packets_per_message = 1;
  remained_packets_per_max_count = 1;
  switch (type) {
    case ComType::All_Reduce:
      this->final_data_size = data_size;
      this->msg_size = data_size / nodes_in_ring; 
      break;
    case ComType::All_Gather:
      this->final_data_size = data_size * nodes_in_ring;
      // std::cout<<"heeeey! here! final data size: "<<this->final_data_size<<"
      // ,nodes in ring: "<<nodes_in_ring<<std::endl;
      this->msg_size = data_size;
      break;
    case ComType::Reduce_Scatter:
      this->final_data_size = data_size / nodes_in_ring;
      this->msg_size = data_size / nodes_in_ring;
      break;
    case ComType::All_to_All:
      this->final_data_size = data_size;
      this->msg_size = data_size / nodes_in_ring;
      // for MATE, there are (N-1) halfring and 1 FORD ring in acceleration period
      if (link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced &&
          ((chunk_stage % 2 == 1 && failure_type != 1 && failure_type != 4) ||
           (chunk_stage % 3 != 0 && (failure_type == 1 || failure_type == 4)))) { // TODO: a better implementation is to make sure every former chunk is finished and then start this multi-dimension accelerated stream
        if (nodes_in_ring % 2 == 0) {
          this->msg_size = ceil((static_cast<double>(data_size) / nodes_num_of_failed_ring) *
                                static_cast<double>(1.0 - static_cast<double>(max_physical_dim_value) / (4 * (nodes_num_of_failed_ring - 1)))) *
                               (1.0 / (static_cast<double>(num_dimensions - 1) + static_cast<double>(nodes_num_of_failed_ring + 1) / (4.0 * nodes_num_of_failed_ring)));
        } else {
          this->msg_size = ceil((static_cast<double>(data_size) / nodes_num_of_failed_ring) * static_cast<double>(1.0 - static_cast<double>(max_physical_dim_value + 1) / (4 * nodes_num_of_failed_ring))) * (1.0 / (static_cast<double>(num_dimensions - 1) + static_cast<double>(nodes_num_of_failed_ring) / (4.0 * (nodes_num_of_failed_ring - 1))));
        }
      } else if (link_failure_scheduling == LinkFailureScheduling::Mate &&
                 ((chunk_stage % 2 == 1 && failure_type != 1 && failure_type != 4) ||
                  (chunk_stage % 3 != 0 && (failure_type == 1 || failure_type == 4)))) {
        if (nodes_in_ring % 2 == 0) {
          this->msg_size = ceil(static_cast<double>(data_size) / nodes_num_of_failed_ring) * 
                               (1.0 / (static_cast<double>(num_dimensions - 1) + static_cast<double>(nodes_num_of_failed_ring + 1) / (4.0 * nodes_num_of_failed_ring)));
        } else {
          this->msg_size = ceil(static_cast<double>(data_size) / nodes_num_of_failed_ring) *
                               (1.0 / (static_cast<double>(num_dimensions - 1) + static_cast<double>(nodes_num_of_failed_ring) / (4.0 * (nodes_num_of_failed_ring - 1))));
        }
      } else if (link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced &&
                 ((chunk_stage % 2 == 0 && failure_type != 1 && failure_type != 4) ||
                  (chunk_stage % 3 == 0 && (failure_type == 1 || failure_type == 4))) &&
                 link_failure_in_this_dimension == 1) {
        if (nodes_in_ring % 2 == 0) {
          this->msg_size = ceil((static_cast<double>(data_size) / nodes_num_of_failed_ring) *
                                (static_cast<double>(max_physical_dim_value) / (4.0 * (nodes_num_of_failed_ring - 1))));
        } else {
          this->msg_size = ceil((static_cast<double>(data_size) / nodes_num_of_failed_ring) *
                                (static_cast<double>(max_physical_dim_value + 1) / (4.0 * nodes_num_of_failed_ring)));
        }
      } else if (link_failure_scheduling == LinkFailureScheduling::Mate &&
                 ((chunk_stage % 2 == 0 && failure_type != 1 && failure_type != 4) ||
                  (chunk_stage % 3 == 0 && (failure_type == 1 || failure_type == 4)))) {
        this->msg_size = data_size / max_physical_dim_value;
      } else if (link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced &&
                 ((chunk_stage % 2 == 0 && failure_type != 1 && failure_type != 4) ||
                  (chunk_stage % 3 == 0 && (failure_type == 1 || failure_type == 4))) &&
                 link_failure_in_this_dimension == 0) {
        this->msg_size = data_size / max_physical_dim_value;
      }
      break;
    default:;
  }
  data_size = this->orig_data_size;
  this->final_data_size = data_size;
}

int HalfRing::get_non_zero_latency_packets() {
  return (nodes_in_ring - 1) * parallel_reduce * 1;
}
void HalfRing::run(EventType event, CallData* data) {
  if (event == EventType::General) {
    free_packets += 1;
    ready();
    iteratable();
  } else if (event == EventType::PacketReceived) {
    total_packets_received++;
    insert_packet(nullptr);
  } else if (event == EventType::StreamInit) {
    for (int i = 0; i < parallel_reduce; i++) {
      insert_packet(nullptr);
    }
  }
}
void HalfRing::release_packets() {
  for (auto packet : locked_packets) {
    packet->set_notifier(this);
  }
  if (NPU_to_MA == true) {
    (new PacketBundle(
         stream->owner,
         stream,
         locked_packets,
         processed,
         send_back,
         msg_size,
         transmition))
        ->send_to_MA();
  } else {
    (new PacketBundle(
         stream->owner,
         stream,
         locked_packets,
         processed,
         send_back,
         msg_size,
         transmition))
        ->send_to_NPU();
  }
  locked_packets.clear();
}
void HalfRing::process_stream_count() {
  if (remained_packets_per_message > 0) {
    remained_packets_per_message--;
  }
  if (id == 0) {
  }
  if (remained_packets_per_message == 0 && stream_count > 0) {
    stream_count--;
    if (stream_count > 0) {
      remained_packets_per_message = 1;
    }
  }
  if (remained_packets_per_message == 0 && stream_count == 0 &&
      stream->state != StreamState::Dead) {
    stream->changeState(StreamState::Zombie);
    if (id == 0) {
      // std::cout<<"stream "<<stream_num<<" changed state to
      // zombie"<<std::endl;
    }
  }
  if (id == 0) {
    // std::cout<<"for stream: "<<stream_num<<" ,total stream count left:
    // "<<stream_count<<std::endl;
  }
}
void HalfRing::process_max_count() {
  if (remained_packets_per_max_count > 0)
    remained_packets_per_max_count--;
  if (remained_packets_per_max_count == 0) {
    max_count--;
    if (id == 0) {
      // std::cout<<"max count is now: "<<max_count<<"stream count is:
      // "<<stream_count<<" , free_packets: "<<free_packets<<std::endl;
    }
    release_packets();
    remained_packets_per_max_count = 1;
  }
}
void HalfRing::reduce() {
  process_stream_count();
  packets.pop_front(); 
  free_packets--;
  total_packets_sent++;
}
bool HalfRing::iteratable() {
  bool cont = !(stream_count == 0 && free_packets == parallel_reduce);
  if (stream_count == 0 &&
      free_packets == (parallel_reduce * 1)) { // && not_delivered==0
    exit();
    return false;
  }
  return true;
}
void HalfRing::insert_packet(Callable* sender) {
  if (!enabled) {
    return;
  }
  if (zero_latency_packets == 0 && non_zero_latency_packets == 0) {
    zero_latency_packets = parallel_reduce * 1;
    non_zero_latency_packets =
        get_non_zero_latency_packets(); //(nodes_in_ring-1)*parallel_reduce*1;
    toggle = !toggle;
  }
  if (zero_latency_packets > 0) {
    packets.push_back(MyPacket(
        stream->current_queue_id,
        current_sender,
        current_receiver)); // vnet Must be changed for alltoall topology
    packets.back().sender = sender;
    locked_packets.push_back(&packets.back());
    processed = false;
    send_back = false;
    NPU_to_MA = true;
    process_max_count();
    zero_latency_packets--;
    return;
  } else if (non_zero_latency_packets > 0) {
    packets.push_back(MyPacket(
        stream->current_queue_id,
        current_sender,
        current_receiver)); // vnet Must be changed for alltoall topology
    packets.back().sender = sender;
    locked_packets.push_back(&packets.back());
    if (comType == ComType::Reduce_Scatter ||
        (comType == ComType::All_Reduce && toggle)) {
      processed = true;
    } else {
      processed = false;
    }
    if (non_zero_latency_packets <= parallel_reduce * 1) {
      send_back = false;
    } else {
      send_back = true;
    }
    NPU_to_MA = false;
    process_max_count();
    non_zero_latency_packets--;
    return;
  }
  Sys::sys_panic("should not inject nothing!");
  //}
}
bool HalfRing::ready() {
  if (stream->state == StreamState::Created ||
      stream->state == StreamState::Ready) {
    stream->changeState(StreamState::Executing);
    // init(); //should be replaced
  }
  if (!enabled || packets.size() == 0 || stream_count == 0 ||
      free_packets == 0) {
    return false;
  }
  MyPacket packet = packets.front();
  sim_request snd_req;
  snd_req.srcRank = id;
  snd_req.dstRank = packet.preferred_dest;
  snd_req.tag = stream->stream_num;
  snd_req.reqType = UINT8;
  snd_req.vnet = this->stream->current_queue_id;
  snd_req.layerNum = layer_num;

  if (this->local_failure_type == 1 || this->local_failure_type == 4) {
    this->local_chunk_stage = (3 * num_dimensions) - stream->phases_to_go.size() - 1;
  } else {
    this->local_chunk_stage = (2 * num_dimensions) - stream->phases_to_go.size() - 1;
  }
  stream->owner->stream_num_ID = stream->stream_num;
  stream->owner->stream_count_ID = this->total_stream_count - this->stream_count;
  stream->owner->chunk_stage = this->local_chunk_stage;
  stream->owner->front_end_sim_send(
    0,
    Sys::dummy_data,
    msg_size,
    UINT8,
    packet.preferred_dest,
    stream->stream_num,
    &snd_req,
    &Sys::handleEvent,
    nullptr); // stream_num+(packet.preferred_dest*50)  
  sim_request rcv_req;
  rcv_req.vnet = this->stream->current_queue_id;
  rcv_req.layerNum = layer_num;
  RecvPacketEventHadndlerData* ehd = new RecvPacketEventHadndlerData(
      stream,
      stream->owner->id,
      EventType::PacketReceived,
      packet.preferred_vnet,
      packet.stream_num);

  uint64_t recv_size;
  if (this->non_uniform_flag!= 0) {
    recv_size = Get_Recv_Size(packet.preferred_src);
  } else {
    recv_size = msg_size; // for uniform All-to-All
  }

  stream->owner->front_end_sim_recv(
      0,
      Sys::dummy_data,
      recv_size,
      UINT8,
      packet.preferred_src,
      stream->stream_num,
      &rcv_req,
      &Sys::handleEvent,
      ehd); // stream_num+(owner->id*50)
  reduce();

  return true;

}
void HalfRing::exit() {
  if (packets.size() != 0) {
    packets.clear();
  }
  if (locked_packets.size() != 0) {
    locked_packets.clear();
  }
  stream->declare_ready();
  stream->owner->proceed_to_next_vnet_baseline((StreamBaseline*)stream);
  return;
}

uint64_t HalfRing::GetMoEDistributionValue(const std::string& filepath, int line_idx, int col_idx) {
  std::ifstream infile(filepath);
  if (!infile.is_open()) {
      std::cerr << "Failed to open file: " << filepath << std::endl;
      perror("System error reason");
      throw std::runtime_error("Failed to open file: " + filepath);
  }

  std::string line;
  int current_line = 0;

  while (std::getline(infile, line)) {
      if (current_line == line_idx) {
          // remove the [] in the line
          if (!line.empty() && line.front() == '[') line.erase(0, 1);
          if (!line.empty() && line.back() == ']') line.pop_back();

          std::stringstream ss(line);
          std::string number_str;
          int current_col = 0;

          while (std::getline(ss, number_str, ',')) {
              if (current_col == col_idx) {
                  try {
                      return std::stoull(number_str);  
                  } catch (const std::exception& e) {
                      std::cerr << "Failed to parse number: " << number_str << " at (" << line_idx << "," << col_idx << "): " << e.what() << std::endl;
                      throw;
                  }
              }
              ++current_col;
          }
          throw std::out_of_range("Column index out of range in line " + std::to_string(line_idx));
      }
      ++current_line;
  }
  throw std::out_of_range("Line index out of range: " + std::to_string(line_idx));
}

uint64_t HalfRing::Get_Recv_Size(int preferred_src) {
  // Galois note: it's hard-coded
  uint64_t dist_val = GetMoEDistributionValue("../../inputs/workload/Non-Uniform-MoE/MoE_Distribution.txt", non_uniform_flag-1, (preferred_src % 8));
  uint64_t src_data_size = dist_val * orig_data_size / 8192;
  uint64_t new_msg_size;
  new_msg_size = src_data_size / nodes_in_ring;

  if (local_link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced &&
      ((local_chunk_stage % 2 == 1 && local_failure_type != 1 && local_failure_type != 4) ||
       (local_chunk_stage % 3 != 0 && (local_failure_type == 1 || local_failure_type == 4)))) { // TODO: a better implementation is to make sure every former chunk is finished and then start this multi-dimension accelerated stream
    if (nodes_in_ring % 2 == 0) {
      // even
      new_msg_size = ceil((static_cast<double>(src_data_size) / local_nodes_num_of_failed_ring) *
                          static_cast<double>(1.0 - static_cast<double>(max_physical_dim_value) / (4 * (local_nodes_num_of_failed_ring - 1)))) *
                         (1.0 / (static_cast<double>(num_dimensions - 1) + static_cast<double>(local_nodes_num_of_failed_ring + 1) / (4.0 * local_nodes_num_of_failed_ring)));
    } else {
      // odd
      new_msg_size = ceil((static_cast<double>(src_data_size) / local_nodes_num_of_failed_ring) *
                          static_cast<double>(1.0 - static_cast<double>(max_physical_dim_value + 1) / (4 * local_nodes_num_of_failed_ring))) *
                         (1.0 / (static_cast<double>(num_dimensions - 1) + static_cast<double>(local_nodes_num_of_failed_ring) / (4.0 * (local_nodes_num_of_failed_ring - 1))));
    }
  } else if (local_link_failure_scheduling == LinkFailureScheduling::Mate &&
             ((local_chunk_stage % 2 == 1 && local_failure_type != 1 && local_failure_type != 4) ||
              (local_chunk_stage % 3 != 0 && (local_failure_type == 1 || local_failure_type == 4)))) {
    if (nodes_in_ring % 2 == 0) {
      new_msg_size = ceil(static_cast<double>(src_data_size) / local_nodes_num_of_failed_ring) *
                         (1.0 / (static_cast<double>(num_dimensions - 1) + static_cast<double>(local_nodes_num_of_failed_ring + 1) / (4.0 * local_nodes_num_of_failed_ring)));
    } else {
      new_msg_size = ceil(static_cast<double>(src_data_size) / local_nodes_num_of_failed_ring) *
                         (1.0 / (static_cast<double>(num_dimensions - 1) + static_cast<double>(local_nodes_num_of_failed_ring) / (4.0 * (local_nodes_num_of_failed_ring - 1))));
    }
  } else if (local_link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced &&
             ((local_chunk_stage % 2 == 0 && local_failure_type != 1 && local_failure_type != 4) ||
              (local_chunk_stage % 3 == 0 && (local_failure_type == 1 || local_failure_type == 4))) &&
             local_link_failure_in_this_dimension == 1) {
    if (nodes_in_ring % 2 == 0) {
      new_msg_size = ceil((static_cast<double>(src_data_size) / local_nodes_num_of_failed_ring) *
                          (static_cast<double>(max_physical_dim_value) / (4.0 * (local_nodes_num_of_failed_ring - 1))));
    } else {
      new_msg_size = ceil((static_cast<double>(src_data_size) / local_nodes_num_of_failed_ring) *
                          (static_cast<double>(max_physical_dim_value + 1) / (4.0 * local_nodes_num_of_failed_ring)));
    }
  } else if (local_link_failure_scheduling == LinkFailureScheduling::Mate &&
             ((local_chunk_stage % 2 == 0 && local_failure_type != 1 && local_failure_type != 4) ||
              (local_chunk_stage % 3 == 0 && (local_failure_type == 1 || local_failure_type == 4)))) {
    new_msg_size = src_data_size / max_physical_dim_value;
  } else if (local_link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced &&
             ((local_chunk_stage % 2 == 0 && local_failure_type != 1 && local_failure_type != 4) ||
              (local_chunk_stage % 3 == 0 && (local_failure_type == 1 || local_failure_type == 4))) &&
             local_link_failure_in_this_dimension == 0) {
    new_msg_size = src_data_size / max_physical_dim_value;
  }
  return new_msg_size;
}

} // namespace AstraSim