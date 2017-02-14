#! /bin/sh

opp_makemake -f; 
make; 
CubeSatNetworks.exe -r 0 \
					-n '.;../inet/src;' \
					-l ../inet/src/libINET.dll \
					--debug-on-errors=false \
					omnetpp.ini;
