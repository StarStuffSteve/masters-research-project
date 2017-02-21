#! /bin/sh

opp_makemake -f --deep -M debug;
make; 
CubeSats.exe -u Cmdenv \
					-r 0 \
					-c "BaseConfig" \
					-n '.;../inet/src;' \
					-l ../inet/src/libINET.dll \
					--record-eventlog=true \
					--debug-on-errors=false \
					omnetpp.ini;
