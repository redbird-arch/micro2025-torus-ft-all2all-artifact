#!/bin/tcsh
python `which scons` debug=1 -j 8 build/Garnet_standalone/gem5.opt
