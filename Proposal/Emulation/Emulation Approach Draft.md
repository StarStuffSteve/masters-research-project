# Emulation Approach Initial Draft

#### Orbital Configuration
- 12 CubeSats (CSs)
- Orbital altitude at approx. 600km (_~150 km higher than TW_)
- Sun synchronized
- Polar orbits
- 4 Orbits serparted by approx. 5 degrees 
- Minimum initial pass straight line inter-orbit distance approx. 50

#### Assumptions
- CSs have no orbital control capabilities

#### Input Data
- Inter-CS range change data from Nodes & Tianwang (within a single orbit)
- Inter-CS range change data from STK (between cubesats of adjacent orbits)
- Down/Uplink access windows for each CubeSat from STK

#### Implementation
- CSs and ground station (GS) modelled by __Docker containers__
- Connections over TCP
- Agents within containers using python [twisted](https://github.com/twisted/twisted) or similar
- Controlled by TC which is informed by range data

![emulation overview](https://github.com/StarStuffSteve/masters-research-project/Proposal/Emulation/emulation_overview.png "Emulation Overview")



