/*
# File name  :    HalfRing.hh
# Author     :    Galois
# Time       :    2024/05/11 16:42:35
*/

#ifndef __HALFRING_HH__
#define __HALFRING_HH__

#include <assert.h>
#include <math.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <list>
#include <map>
#include <sstream>
#include <tuple>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include "Algorithm.hh"
#include "astra-sim/system/Common.hh"
#include "astra-sim/system/MemBus.hh"
#include "astra-sim/system/MyPacket.hh"
#include "astra-sim/system/topology/RingTopology.hh"

namespace AstraSim {
class HalfRing : public Algorithm {
 public:
  // enum class Type{AllReduce,AllGather,ReduceScatter,AllToAll};
  // RingTopology::Direction dimension;
  RingTopology::Dimension dimension;
  RingTopology::Direction direction;
  MemBus::Transmition transmition;
  int zero_latency_packets;
  int non_zero_latency_packets;
  int id;
  int current_receiver;
  int current_sender;
  int nodes_in_ring;
  int stream_count;
  int max_count;
  int remained_packets_per_max_count;
  int remained_packets_per_message;
  int parallel_reduce;
  InjectionPolicy injection_policy;
  std::list<MyPacket> packets;
  bool toggle;
  long free_packets;
  long total_packets_sent;
  long total_packets_received;
  uint64_t msg_size;
  std::list<MyPacket*> locked_packets;
  bool processed;
  bool send_back;
  bool NPU_to_MA;
  int total_stream_count;
  int local_chunk_stage;
  int local_failure_type;
  int num_dimensions;
  int max_physical_dim_value;
  int non_uniform_flag;
  uint64_t orig_data_size;
  LinkFailureScheduling local_link_failure_scheduling;
  int local_nodes_num_of_failed_ring;
  int local_link_failure_in_this_dimension;
  HalfRing(
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
      int non_uniform_flag);
  virtual void run(EventType event, CallData* data);
  void process_stream_count();
  // void call(EventType event,CallData *data);
  void release_packets();
  virtual void process_max_count();
  void reduce();
  bool iteratable();
  virtual int get_non_zero_latency_packets();
  void insert_packet(Callable* sender);
  bool ready();
  uint64_t GetMoEDistributionValue(const std::string& filepath, int line_idx, int col_idx);
  uint64_t Get_Recv_Size(int preferred_src);
  void exit();
};
} // namespace AstraSim
#endif