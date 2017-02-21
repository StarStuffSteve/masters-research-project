#! /bin/sh

opp_makemake -f --deep -M debug;
make; 
CubeSats.exe -u Qtenv \
					-r 0 \
					-n '.;../inet/src;' \
					-l ../inet/src/libINET.dll \
					--debug-on-errors=false \
					omnetpp.ini;      
