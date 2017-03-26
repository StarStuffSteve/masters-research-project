#! /bin/sh

opp_makemake -f --deep -M debug;
make; 
CubeSats.exe -u Cmdenv \
					-c "Clusters" \
					-n '.;../inet/src;' \
					-l ../inet/src/libINET.dll \
					--record-eventlog=false \
					--debug-on-errors=false \
					--**.cmdenv-log-level="FATAL"\
					--cmdenv-status-frequency=10s \
					omnetpp.ini;
