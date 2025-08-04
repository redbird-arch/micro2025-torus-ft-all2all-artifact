/*
# File name  :    OneStage.hh
# Author     :    Galois
# Time       :    2024/10/09 14:54:18
*/
#ifndef __ONESTAGE_HH__
#define __ONESTAGE_HH__

#include "astra-sim/system/Common.hh"
#include "astra-sim/system/Sys.hh"
#include <vector>

namespace AstraSim {

class OneStage {
public:
  OneStage(Sys* sys);
  std::vector<int> get_chunk_scheduling();
  Sys* sys;
};

} // namespace AstraSim

#endif // __ND_TORUS_RING_HH__
