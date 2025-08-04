/*
# File name  :    ND_Torus_Ring_AlltoAll_AllReduce.hh
# Author     :    Galois
# Time       :    2024/02/29 21:26:58
*/

#ifndef __ND_Torus_Ring_AlltoAll_AllReduce__
#define __ND_Torus_Ring_AlltoAll_AllReduce__

#include "astra-sim/system/Common.hh"
#include "astra-sim/system/Sys.hh"
#include <vector>

namespace AstraSim {

class ND_Torus_Ring_AlltoAll_AllReduce {
public:
  ND_Torus_Ring_AlltoAll_AllReduce(Sys* sys);
  std::vector<int> get_chunk_scheduling(long long chunk_id, LinkFailureScheduling link_failure_schedule);
  Sys* sys;
  int nDimensions;
};

} // namespace AstraSim

#endif // __ND_Torus_Ring_AlltoAll_AllReduce__
