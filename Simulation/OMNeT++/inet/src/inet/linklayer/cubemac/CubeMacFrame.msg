// A CubeMAC packet format definition

cplusplus {{
    #include "inet/linklayer/base/MACFrameBase_m.h"
}}

namespace inet;

class noncobject MACAddress;
class MACFrameBase;

packet CubeMacFrame extends MACFrameBase
{
	bool isLastPacket; // Used to signal that sender has no more packets to send after this one
    bool containsData; // used to signal other nodes that there is no data and no data coming
    int clusterId; // Identifies the origin cluster of this message
    simtime_t arrivalTimeMac; // Used to compute access delay
}
