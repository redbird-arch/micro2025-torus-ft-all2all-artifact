/*
# File name  :    MultiRing.hh
# Author     :    Galois
# Time       :    2024/02/27 13:41:03
*/
#ifndef __ND_TORUS_RING_HH__
#define __ND_TORUS_RING_HH__

#include "astra-sim/system/Common.hh"
#include "astra-sim/system/Sys.hh"
#include <vector>

namespace AstraSim {

class ND_Torus_Ring {
public:
  ND_Torus_Ring(Sys* sys);
  std::vector<int> get_chunk_scheduling(long long chunk_id, LinkFailureScheduling link_failure_schedule, int failure_type);
  Sys* sys;
  int nDimensions;
};

} // namespace AstraSim

#endif // __ND_TORUS_RING_HH__
