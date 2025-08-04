#!/bin/bash

# generate Synthetic_Speedup figure (Figure 11 in the paper)
python Synthetic_Speedup.py && echo "Finish Figure 11!"
# generate Synthetic_Bandwidth figure (Figure 12 in the paper)
python Synthetic_Bandwidth.py && echo "Finish Figure 12!"
# generate Synthetic_Utilization figure (Figure 13 in the paper)
python Synthetic_Utilization.py && echo "Finish Figure 13!"
# generate Scalability figure (Figure 14 in the paper)
python Scalability.py && echo "Finish Figure 14!"
# generate Performance_comparison_with_Google_routing_method figure (Figure 15(a) in the paper)
python Google_444.py && echo "Finish Figure 15(a)!"
# generate Performance_comparison_with_Google_routing_method figure (Figure 15(b) in the paper)
python Google_844.py && echo "Finish Figure 15(b)!"
# generate Real_Model_Performance figure (Figure 16 in the paper)
python RealModel_exp.py && echo "Finish Figure 16!"
# generate Non_Uniform_All_to_All figure (Figure 17(a) in the paper)
python Non_Uniform_Point1.py && echo "Finish Figure 17(a)!"
# generate Non_Uniform_All_to_All figure (Figure 17(b) in the paper)
python Non_Uniform_Point2.py && echo "Finish Figure 17(b)!"
# generate Multi_Failure figure (Figure 18(b) in the paper)
python Google_MultiFault.py && echo "Finish Figure 18(b)!"
# generate Real_Machine_Test figure (Figure 19(a) in the paper)
python Real_Machine.py && echo "Finish Figure 19(a)!"
