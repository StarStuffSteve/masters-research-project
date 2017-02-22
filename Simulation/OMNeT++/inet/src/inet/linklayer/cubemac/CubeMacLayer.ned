//
//  LMACLayer.cc
//  BMAC for MF 2.02, omnetpp 4
//
//  Created by Anna Foerster on 10/10/08.
//  Copyright 2008 Universita della Svizzera Italiana. All rights reserved.
//
//  Coverted to OMNeT++ 4 by Rudolf Hornig
//
//  Converted to MiXiM by Kapourniotis Theodoros
//

package inet.linklayer.cubemac;

import inet.linklayer.base.MACProtocolBase;
import inet.linklayer.contract.IMACProtocol;

//
// Implementation of L-MAC (Lightweight Medium Access Protocol for
// Wireless Sensor Networks)
//
// See C++ documentation for details
//

// --- MACProtocolBase is parent of IdealMac
simple CubeMacLayer extends MACProtocolBase like IMACProtocol
{
    parameters:
        // 
        // --- Added
        //
        double startTime @unit(s) = default(this.slotDuration);
        int masterId = default(0);
        bool isSlave = default(false);
        int slaveId = default(0);
        int slavesInCluster = default(0);
        // ---
        
        string address = default("auto"); 
        // MAC address as hex string (12 hex digits), or
        // "auto". "auto" values will be replaced by
        // a generated MAC address in init stage 0.
        
        // how long is a slot?
        double slotDuration @unit(s) = default(100ms);
        
        int headerLength @unit(B) = default(10B);
        
        int mtu @unit("B") = default(0B);
        
        // length of MAC header
        double queueLength = default(10);
        
        // default channel
        double defaultChannel = default(0);
        
        // bit rate
        double bitrate @unit(bps) = default(19200bps);
        
        // level at which we consider medium busy FIXME not used currently
        // double busyRSSI;
        
        // --- Should never use the default
        int numSlots = default(64);
        
        // The path to the Radio module
        string radioModule = default("^.radio");     //FIXME remove default value

        @class(CubeMacLayer);
}
