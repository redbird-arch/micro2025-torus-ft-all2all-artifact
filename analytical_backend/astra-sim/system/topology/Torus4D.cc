/*
# File name  :    Torus4D.cc
# Author     :    Galois
# Time       :    2024/01/29 17:39:27
*/

#include "Torus4D.hh"
namespace AstraSim {
Torus4D::Torus4D(int id, int total_nodes, int local_dim, int vertical_dim, int horizontal_dim) {
  int deep_mod = local_dim * vertical_dim * horizontal_dim; // as the mod base when calculating deepID
  int deep_dim = total_nodes / deep_mod;
  local_dimension = new RingTopology(
      RingTopology::Dimension::Local, id, local_dim, id % local_dim, 1);
  vertical_dimension = new RingTopology(
      RingTopology::Dimension::Vertical,
      id,
      vertical_dim,
      (id % deep_mod) / (local_dim * horizontal_dim),
      local_dim * horizontal_dim);
  horizontal_dimension = new RingTopology(
      RingTopology::Dimension::Horizontal,
      id,
      horizontal_dim,
      ((id % deep_mod) / local_dim) % horizontal_dim,
      local_dim);
  deep_dimension = new RingTopology(
      RingTopology::Dimension::Deep,
      id,
      deep_dim,
      id % deep_mod,
      deep_mod);
}
Torus4D::~Torus4D() {
  delete local_dimension;
  delete vertical_dimension;
  delete horizontal_dimension;
  delete deep_dimension;
};
int Torus4D::get_num_of_dimensions() {
  return 4;
}
int Torus4D::get_num_of_nodes_in_dimension(int dimension) {
  if (dimension == 0) {
    return local_dimension->get_num_of_nodes_in_dimension(0);
  } else if (dimension == 1) {
    return vertical_dimension->get_num_of_nodes_in_dimension(0);
  } else if (dimension == 2) {
    return horizontal_dimension->get_num_of_nodes_in_dimension(0);
  } else if (dimension == 3) {
    return deep_dimension->get_num_of_nodes_in_dimension(0);
  }
  return -1;
}
BasicLogicalTopology* Torus4D::get_basic_topology_at_dimension(
    int dimension,
    ComType type) {
  if (dimension == 0) {
    return local_dimension;
  } else if (dimension == 1) {
    return vertical_dimension;
  } else if (dimension == 2) {
    return horizontal_dimension;
  } else if (dimension == 3) {
    return deep_dimension;
  }
  return NULL;
}
} // namespace AstraSim