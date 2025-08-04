/*
# File name  :    ND_Torus_Ring_AlltoAll_AllReduce.cc
# Author     :    Galois
# Time       :    2024/02/29 21:26:39
*/
#include "ND_Torus_Ring_AlltoAll_AllReduce.hh"

namespace AstraSim {

ND_Torus_Ring_AlltoAll_AllReduce::ND_Torus_Ring_AlltoAll_AllReduce(Sys* sys) {
  this->sys = sys;
  this->nDimensions = sys->physical_dims.size();
}

std::vector<int> ND_Torus_Ring_AlltoAll_AllReduce::get_chunk_scheduling(long long chunk_id, LinkFailureScheduling link_failure_scheduling) {
  std::vector<int> schedule(nDimensions);
  std::vector<int> schedule_MATE(2*nDimensions);
  int start = chunk_id % nDimensions;
  
  
  if (link_failure_scheduling == LinkFailureScheduling::Mate ||
      link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced) {
    for (int i = 0; i < nDimensions; ++i) {
      schedule_MATE[2 * i] = (start + i) % nDimensions;     
      schedule_MATE[2 * i + 1] = (start + i) % nDimensions; 
    }
    return schedule_MATE;
  } else {
    for (int i = 0; i < nDimensions; ++i) {
      schedule[i] = (start + i) % nDimensions;
    }
    return schedule;
  }
}

} // namespace AstraSim
