/*
# File name  :    Torus2D.hh
# Author     :    Galois
# Time       :    2024/01/29 16:58:18
*/

#ifndef __TORUS2D_HH__
#define __TORUS2D_HH__

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
#include "ComplexLogicalTopology.hh"
#include "RingTopology.hh"
#include "astra-sim/system/Common.hh"

namespace AstraSim {
class Torus2D : public ComplexLogicalTopology {
 public:
  RingTopology* local_dimension;
  RingTopology* vertical_dimension;
  Torus2D(int id, int total_nodes, int local_dim);
  ~Torus2D();
  int get_num_of_dimensions() override;
  int get_num_of_nodes_in_dimension(int dimension) override;
  BasicLogicalTopology* get_basic_topology_at_dimension(
      int dimension,
      ComType type) override;
};
} // namespace AstraSim
#endif