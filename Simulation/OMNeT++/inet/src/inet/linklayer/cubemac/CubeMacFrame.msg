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
    int clusterId; // Identifies the origin cluster of this message
}
