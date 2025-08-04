/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#ifndef __HIERARCHICALTOPOLOGY_HH__
#define __HIERARCHICALTOPOLOGY_HH__

#include "HierarchicalTopologyConfig.hh"
#include "Topology.hh"
#include "TopologyConfig.hh"


namespace Analytical {
class HierarchicalTopology : public Topology {
 public:
  using TopologyList = HierarchicalTopologyConfig::TopologyList;
  using DimensionType = HierarchicalTopologyConfig::DimensionType;
  double bandwidth_scalar;

  HierarchicalTopology(
      TopologyConfigs& configs,
      HierarchicalTopologyConfig hierarchy_config) noexcept;

  ~HierarchicalTopology() noexcept override;

  Bandwidth getNpuTotalBandwidthPerDim(int dimension) const noexcept override;

  std::pair<double, int> send(
      NpuId src,
      NpuId dest,
      PayloadSize payload_size,
      int stream_num_ID,
      int stream_count_ID,
      int link_failure_scheduling_flag,
      int chunk_stage,
      int failure_type) noexcept override; // baseline: 0, mate: 1, mate_enhanced: 2
      
 private:
  HierarchicalTopologyConfig hierarchy_config;
  std::vector<int> link_failure_vector; // record the link failure num per dim

  Latency linkLatency(int dimension, int hops_count) const noexcept;

  NpuAddress npuIdToAddress(NpuId npu_id) const noexcept override;
  NpuId npuAddressToId(NpuAddress npu_address) const noexcept override;
};
} // namespace Analytical

#endif
