# Simulation Side Notes

### NS3 Tutorial 3 - Wireless
- "ns-3 provides a set of 802.11 models that attempt to provide an accurate MAC-level implementation of the 802.11 specification and a “not-so-slow” PHY-level model of the 802.11a specification"
- wifi has wifi channel and phy 'object'
- Treat masters and S2G nodes as access points?
- Mobility suite is class
- "One thing that can surprise some users is the fact that the simulation we just created will never “naturally” stop. This is because we asked the wireless access point to generate beacons. It will generate beacons forever, and this will result in simulator events being scheduled into the future indefinitely, so we must tell the simulator to stop even though it may have beacon generation events scheduled"

### Tracing Tutorial 5 + 
- "you must ensure that the target of a Config::Connect command exists before trying to use it."
- "In particular, an ns-3 Socket is a dynamic object often created by Applications to communicate between Nodes."
- fifth: "Create the dynamic object at configuration time, hook it then, and give the object to the system to use during simulation time"
- "It is the responsibility of the Application to keep scheduling the chain of events, so the next lines call ScheduleTx to schedule another transmit event (a SendPacket) until the Application decides it has sent enough."
- "all device helpers in the system will have all of the PCAP trace methods available; and these methods will all work in the same way across devices if the device implements EnablePcapInternal correctly"
- Promiscuous is a common flag amoung trace enabling functions that typically defaults to false
- helper.EnablePcap ("prefix", "server/ath0"); // Selecting a pairing
- helper.EnablePcapAll ("prefix"); // All devices with same type as helper


