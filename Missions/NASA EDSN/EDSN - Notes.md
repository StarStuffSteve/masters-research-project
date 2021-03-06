# Edison Demonstration of Smallsat Networks (EDSN)

#### Overview

- Eight satellites carrying a science experiment and capable of inter-communication. 
- Lost on the ORS-4 mission. 
- 1.5U and 2kg w/ no propulsion and would have drifted apart in orbit naturally
- Take turns in downlink
- Data is replicated across the swarm
- Based out of Ames
- Planned 60 day duration
- Will drift apart once deployed

#### Technology and Capabilities

- Android innards based off the earlier PhoneSat 2.0 bus
- Data exchanged using ‘hub and spoke’ topology
- The hub (captain) is switched every 25 hours
- Hub collects all data, aggregates and arranges for transmit
- “The satellites will continue recording science data and downlinking to the ground station even after they exceed inter-satellite communication range”
- Science payload measures energies of nearby charged particles (EPISEM)
- There is a degree of attitude and orbital control/determination
- Data bus between subsystems is simple ACK, ReTrans setup
- Hardware Watchdog w/ timer

###### Comms
- “Microhard Systems Inc MHX2420 transceiver” for S2G S-Band (Specced at 50km) FHSS 38.4kbps
- Crosslink performed with “AstroDev Lithium 1” 9.6kbps (Contradiction) using AX.25 at 1W <20km ISL range
- Captain pings each LT (Lieutenant) six times (common frequency so pings have IDs)
- Only one LT talks at any one time
- No ACK or NACK between CPT and LT, no guarantee of successful TX (Future Work)
- CPT puts LT packets onto FIFO queue which is pushed onto a LIFO stack for downlink (Priority is LT health data)
- There is downlink stack for each LT
- GPS data is used to sync clocks (UTC) and start CPT rotation
- UTC time used to coordinate wake ups of the LTs
- Only uplink is to disable all communications (FCC requirement)

###### Flight Software Release 6.5
- Mission Simulations were performed with the vehicles in a lab No sign of SW based simulation or planning
- Papers states that tests indicated expected performance
- Nothing about how captain handover happens