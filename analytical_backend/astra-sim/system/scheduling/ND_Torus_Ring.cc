/*
# File name  :    MultiRing.cc
# Author     :    Galois
# Time       :    2024/02/27 13:41:07
*/
#include "ND_Torus_Ring.hh"

namespace AstraSim {

ND_Torus_Ring::ND_Torus_Ring(Sys* sys) {
  this->sys = sys;
  this->nDimensions = sys->physical_dims.size();
}

std::vector<int> ND_Torus_Ring::get_chunk_scheduling(long long chunk_id, LinkFailureScheduling link_failure_scheduling, int failure_type) {
  std::vector<int> schedule(nDimensions);
  std::vector<int> schedule_MATE(2*nDimensions);
  std::vector<int> schedule_Multi_Failure(3*nDimensions);
  int start = chunk_id % nDimensions;
  
  
  if ((link_failure_scheduling == LinkFailureScheduling::Mate ||
       link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced) &&
       failure_type != 1 && failure_type != 4) {
    for (int i = 0; i < nDimensions; ++i) {
      schedule_MATE[2 * i] = (start + i) % nDimensions;     
      schedule_MATE[2 * i + 1] = (start + i) % nDimensions; 
    }
    return schedule_MATE;
  } else if ((link_failure_scheduling == LinkFailureScheduling::Mate ||
              link_failure_scheduling == LinkFailureScheduling::Mate_Enhanced) &&
             (failure_type == 1 || failure_type == 4)){
    for (int i = 0; i < nDimensions; ++i) {
      schedule_Multi_Failure[3 * i] = (start + i) % nDimensions;     
      schedule_Multi_Failure[3 * i + 1] = (start + i) % nDimensions; 
      schedule_Multi_Failure[3 * i + 2] = (start + i) % nDimensions; 
    }
    return schedule_Multi_Failure;
  } else {
    for (int i = 0; i < nDimensions; ++i) {
      schedule[i] = (start + i) % nDimensions;
    }
    return schedule;
  }
}

} // namespace AstraSim

