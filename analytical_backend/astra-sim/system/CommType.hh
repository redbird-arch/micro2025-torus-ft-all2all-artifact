#ifndef COMMUNICATION_TYPES_HH
#define COMMUNICATION_TYPES_HH

namespace AstraSim {
enum class ComType {
  None,
  Reduce_Scatter,
  All_Gather,
  All_Reduce,
  All_to_All,
  All_Reduce_All_to_All
};
}

#endif
