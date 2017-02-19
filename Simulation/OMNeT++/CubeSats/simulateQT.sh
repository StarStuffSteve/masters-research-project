#! /bin/sh

opp_makemake -f --deep -M debug \
				-P '.';
# -Idir '/d/Amazon Drive/Sync/Home/TCD/MAI/Project/GitHub/Simulation/OMNeT++/inet/src' \
# ^ permission denied ^

# -l '../inet/out/gcc-debug/libINET.dll' \
# ^ many many errors ^
make; 
CubeSats.exe -r 0 \
					-n '.;../inet/src;' \
					-l ../inet/src/libINET.dll \
					--debug-on-errors=false \
					omnetpp.ini;      
