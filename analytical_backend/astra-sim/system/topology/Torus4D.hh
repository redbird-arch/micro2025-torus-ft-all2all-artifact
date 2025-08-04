/*
# File name  :    Torus4D.hh
# Author     :    Galois
# Time       :    2024/01/29 17:39:34
*/

#ifndef __TORUS4D_HH__
#define __TORUS4D_HH__

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
class Torus4D : public ComplexLogicalTopology {
 public:
  RingTopology* local_dimension;
  RingTopology* vertical_dimension;
  RingTopology* horizontal_dimension;
  RingTopology* deep_dimension;
  Torus4D(int id, int total_nodes, int local_dim, int vertical_dim, int horizontal_dim);
  ~Torus4D();
  int get_num_of_dimensions() override;
  int get_num_of_nodes_in_dimension(int dimension) override;
  BasicLogicalTopology* get_basic_topology_at_dimension(
      int dimension,
      ComType type) override;
};
} // namespace AstraSim
#endif