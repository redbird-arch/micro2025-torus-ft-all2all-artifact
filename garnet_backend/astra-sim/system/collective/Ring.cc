/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#include "Ring.hh"
#include "astra-sim/system/PacketBundle.hh"
#include "astra-sim/system/RecvPacketEventHadndlerData.hh"
namespace AstraSim {
Ring::Ring(
    ComType type,
    int id,
    int layer_num,
    RingTopology* ring_topology,
    uint64_t data_size,
    RingTopology::Direction direction,
    InjectionPolicy injection_policy,
    std::vector<int> physical_dims,
    bool boost_mode,
    int non_uniform_flag)
    : Algorithm(layer_num) {
  this->comType = type;
  this->id = id;
  this->logicalTopology = ring_topology;
  this->data_size = data_size;
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
  this->name = Name::Ring;
  this->enabled = true;
  this->dim = ring_topology->dim;
  this->update_counter = 1;
  this->physical_dims = physical_dims;
  this->non_uniform_flag = non_uniform_flag; 
  
  // Galois change: enable the non-uniform All-to-All
  if (non_uniform_flag != 0) {
    // Galois note: it's hard-coded
    data_size = GetMoEDistributionValue("../../../inputs/workload/Non-Uniform/MoE_Distribution.txt", non_uniform_flag-1, (id % 8)) * data_size / 8192; // Galois change: get the MoE distribution value from external txt file  
  }

  if (boost_mode) {
    this->enabled = ring_topology->is_enabled();
  }
  if (ring_topology->dimension == RingTopology::Dimension::Local) {
    transmition = MemBus::Transmition::Fast;
  } else {
    transmition = MemBus::Transmition::Usual;
  }

  this->total_nodes = 1;
  this->index_offset = 1;
  for (int i = 0; i < physical_dims.size(); ++i){
    this->total_nodes *= physical_dims[i];
  }

  switch (type) {
    case ComType::All_Reduce:
      stream_count = 2 * (nodes_in_ring - 1);
      break;
    case ComType::All_to_All:
      this->stream_count = this->total_nodes-1;
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
      break;
    default:
      stream_count = nodes_in_ring - 1;
  }
  if (type == ComType::All_to_All || type == ComType::All_Gatehr) {
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
    case ComType::All_Gatehr:
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
      this->msg_size = data_size / this->total_nodes; 
      break;
    default:;
  }
}
int Ring::get_non_zero_latency_packets() {
  return (nodes_in_ring - 1) * parallel_reduce * 1;
}
void Ring::run(EventType event, CallData* data) {
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
void Ring::release_packets() {
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
void Ring::process_stream_count() {
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
void Ring::process_max_count() {
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
void Ring::reduce() {
  process_stream_count();
  packets.pop_front();
  free_packets--;
  total_packets_sent++;
  // not_delivered++;
}
bool Ring::iteratable() {
  if (stream_count == 0 &&
      free_packets == (parallel_reduce * 1)) { // && not_delivered==0
    exit();
    return false;
  }
  return true;
}

void Ring::insert_packet(Callable* sender) {
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
    for (int i = 0; i < this->total_nodes/2; ++i) { 
      async_update_src_dst();
      MyPacket my_packet = MyPacket(this->update_counter - 1, current_sender, current_receiver);
      packets.push_back(my_packet);
      packets.back().sender = sender;
      locked_packets.push_back(&packets.back());
  
      processed = false;
      send_back = false;
      NPU_to_MA = true;
  
      process_max_count();
    }
    zero_latency_packets--; 
    async_update_src_dst();
    while (this->update_counter <= this->total_nodes) { 
      MyPacket my_packet = MyPacket(this->update_counter-1, current_sender, current_receiver);
      packets.push_back(my_packet); // vnet Must be changed for alltoall topology
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
      async_update_src_dst();
      if (non_zero_latency_packets > 1) {
        non_zero_latency_packets--;
      }
    } 
    return;

  } else if (non_zero_latency_packets > 0) {
    while (this->update_counter <= this->total_nodes) { 
      MyPacket my_packet = MyPacket(this->update_counter-1, current_sender, current_receiver);
      packets.push_back(my_packet); // vnet Must be changed for alltoall topology
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
      async_update_src_dst();
      if (non_zero_latency_packets > 1) {
        non_zero_latency_packets--;
      }
    } 
    
    if (total_packets_received == this->total_nodes-1 && this->update_counter == this->total_nodes+1) { 
      MyPacket my_packet = MyPacket(0, current_sender, current_receiver);
      packets.push_back(my_packet); // vnet Must be changed for alltoall topology
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
      async_update_src_dst();
      non_zero_latency_packets = 0;
    }
    return;
  }
  Sys::sys_panic("should not inject nothing!"); 
  //}
}


bool Ring::ready() {
  if (stream->state == StreamState::Created ||
      stream->state == StreamState::Ready) {
    stream->changeState(StreamState::Executing);
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
  snd_req.vnet = packet.preferred_vnet;
  snd_req.layerNum = layer_num;
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
  rcv_req.vnet = packet.preferred_vnet;
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
void Ring::exit() {
  if (packets.size() != 0) {
    packets.clear();
  }
  if (locked_packets.size() != 0) {
    locked_packets.clear();
  }
  stream->declare_ready();
  stream->owner->proceed_to_next_vnet_baseline((StreamBaseline*)stream);
  // delete this;
  return;
}

void Ring::async_update_src_dst() { 
  // Pairwise algorithm for long vector and power-of-2 scenario
  int target = id ^ this->update_counter;
  this->current_sender = target;
  this->current_receiver = target;
  this->update_counter += 1;
}    


std::vector<int> Ring::id_to_coordinate(int id) { 
    std::vector<int> coordinate(this->physical_dims.size(), 0);
    for (size_t i = 0; i < this->physical_dims.size(); ++i) {
        coordinate[i] = id % this->physical_dims[i];
        id /= this->physical_dims[i];
    }
    return coordinate;
}

// Galois change: get a function to read the MoE distribution from external txt file
uint64_t Ring::GetMoEDistributionValue(const std::string& filepath, int line_idx, int col_idx) {
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

uint64_t Ring::Get_Recv_Size(int preferred_src) {
  // Galois note: it's hard-coded
  uint64_t dist_val = GetMoEDistributionValue("../../../inputs/workload/Non-Uniform/MoE_Distribution.txt", non_uniform_flag-1, (preferred_src % 8));
  uint64_t src_data_size = dist_val * data_size / 8192;
  uint64_t new_msg_size;
  new_msg_size = src_data_size / this->total_nodes;
  return new_msg_size;
}

} 