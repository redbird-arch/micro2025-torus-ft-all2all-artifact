/*
# File name  :    Torus2D.cc
# Author     :    Galois
# Time       :    2024/01/29 16:55:57
*/

#include "Torus2D.hh"
namespace AstraSim {
Torus2D::Torus2D(int id, int total_nodes, int local_dim) {
  int vertical_dim = total_nodes / local_dim;
  local_dimension = new RingTopology(
      RingTopology::Dimension::Local, id, local_dim, id % local_dim, 1);
  vertical_dimension = new RingTopology(
      RingTopology::Dimension::Vertical, // DimType
      id,  // global id
      vertical_dim, // how many nodes in this dimension 
      id / local_dim, // which row does this NodeID reside
      local_dim); // the distance between neighbours in this dimension
}
Torus2D::~Torus2D() {
  delete local_dimension;
  delete vertical_dimension;
};
int Torus2D::get_num_of_dimensions() {
  return 2;
}
int Torus2D::get_num_of_nodes_in_dimension(int dimension) {
  if (dimension == 0) {
    return local_dimension->get_num_of_nodes_in_dimension(0);
  } else if (dimension == 1) {
    return vertical_dimension->get_num_of_nodes_in_dimension(0);
  } 
  return -1;
}
BasicLogicalTopology* Torus2D::get_basic_topology_at_dimension(
    int dimension,
    ComType type) {
  if (dimension == 0) {
    return local_dimension;
  } else if (dimension == 1) {
    return vertical_dimension;
  }
  return NULL;
}
} // namespace AstraSim