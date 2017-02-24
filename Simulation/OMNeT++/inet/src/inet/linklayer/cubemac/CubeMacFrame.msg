// A CubeMAC packet format definition
//
// @author Anna Foerster

cplusplus {{
    #include "inet/linklayer/base/MACFrameBase_m.h"
}}

namespace inet;

class noncobject MACAddress;
class MACFrameBase;

packet CubeMacFrame extends MACFrameBase
{
//    int numIncoming; // used to signal other nodes of the number of incoming data packet *following* this one 
    bool containsData; // used to signal other nodes that there is no data and no data coming
    int clusterId; // Identifies the origin cluster of this message
    int senderId; // Identifies sender
}
