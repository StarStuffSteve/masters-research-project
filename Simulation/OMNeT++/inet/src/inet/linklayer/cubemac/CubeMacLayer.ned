package inet.linklayer.cubemac;

import inet.linklayer.base.MACProtocolBase;
import inet.linklayer.contract.IMACProtocol;

// --- MACProtocolBase is parent of IdealMac
simple CubeMacLayer extends MACProtocolBase like IMACProtocol
{
    parameters:
        // 
        // --- Added
        //
        double startTime @unit(s) = default(this.slotDuration);
        
        int clusterId = default(0);
        
        bool isSlave = default(false);
        
        bool isGround = default(false);
        
        int slavesInCluster = default(0);
        
        double timeoutDuration @unit(s) = default((this.slotDuration/10));
        
        double slotPadding @unit(s) = default((this.slotDuration/10));
        
        //  Results, Stats, etc.
        
        
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
