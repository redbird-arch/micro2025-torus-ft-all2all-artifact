/*
# File name  :    OneStage.cc
# Author     :    Galois
# Time       :    2024/10/09 14:56:47
*/
#include "OneStage.hh"

namespace AstraSim {

OneStage::OneStage(Sys* sys) {
  this->sys = sys;
}

std::vector<int> OneStage::get_chunk_scheduling() {
  std::vector<int> schedule(1);
  return schedule;
}

} // namespace AstraSim