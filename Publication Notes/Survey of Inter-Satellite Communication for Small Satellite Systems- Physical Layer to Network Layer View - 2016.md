## Survey of Inter-Satellite Communication for Small Satellite Systems: Physical Layer to Network Layer View

Paper available [here](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=7466793)

### Introduction
- Includes general information will be a useful reference when writing thesis intro
- A handy dandy abbrev table is included

### Background

##### A. Satellite Formation Flying
- Ground based control (km separation) v. autonomous control (sub-km separation)
- Three most common formations:
    + __Trailing__: Single orbit fixed distances
    + __Cluster__: Multi-orbit holding in-orbit & inter-orbit topology
    + __Constellation__: Typically refers to fully earth coverage
-  __Swarm__: "satellite swarm is defined as a set of agents which are identical and self organizing that communicate directly or indirectly and achieve a mission objective by their collective behavior [35](http://repository.tudelft.nl/islandora/object/uuid:b471eea7-5261-4dbf-b388-d410cbde28a5?collection=research)."
- __Factionated__: "the functionalities of a single large satellite are distributed across multiple modules, which interact using wireless links [35](http://repository.tudelft.nl/islandora/object/uuid:b471eea7-5261-4dbf-b388-d410cbde28a5?collection=research)."

##### B. Inter-Satellite Communications (ISC)

- "The current state of the art for small satellite communications is a one hop link between satellite and ground stations" - _future missions cited_
- Explanation of the OSI model
- Paper focuses on the Phy, Link & Network layers

### OSI Model Layers

##### A. Physical Layer

###### 1) Frequency Allocation and Data Rate:

- "existing spectrum should be sufficient to meet expected demands until 2020 [47](https://archive.org/details/nasa_techdoc_20030025224)."
- Maximizing data rates: Increasing bandwidth is a better option to increasing S2N ratio. Higher data rates achieved by transmitting to ground in bursts rather than continually. [49](http://www.lr.tudelft.nl/fileadmin/Faculteit/LR/Organisatie/Afdelingen_en_Leerstoelen/Afdeling_SpE/Space_Systems_Eng./Publications/2008/doc/IAC-08.B2.1.3_Milliano_TOWARDS_NEXT_GENERATION_OF_NANOSAT_COMMS.pdf)
- Maximum data rate equation given on _page 2448_ taken from [47]
- "The majority of cubesat programs utilize UHF/VHF transceivers for downlink communication with no inter-satellite links [51](http://blog.couble.ovh/assets/muri_mcnair_2012_cs_intersat_link_cubesat.pdf)."
- Higher frequency -> more power -> smaller antenna -> higher potential bandwidth

###### 2) Modulation and Coding Schemes:
- "Binary Phase Shift Keying (BPSK) is presently the preferable choice for small satellites because these __coherent systems__ require the least amount of power to support a given throughput and bit error rate" (FSK requiring more power to support comparable throughput)
- QPSK and offset-QPSK potentially twice as efficient as BPSK overcome channel phase distortion w/ differential-PSK
- More constellations more power, trade-off between spectral efficiency and power requirements
- FEC recommended (Parity, Viterbi, Low Density Parity Check (LDPC) code)
- See [53](https://www.hindawi.com/journals/tswj/2013/509508/) on space LDPC and [48 (nice text book)](https://www.amazon.com/Mission-Analysis-Design-Technology-Library/dp/1881883108)  for more on modulations and coding schemes

###### 3) Link Design:
- __Outlines a good procedure to follow for simulated link design__
- Process of link design: Identify communication requirements, Infer ISL and up/downlink data rates, design each link, communication payload resource restrictions (form, power and mass)
- [48](https://www.amazon.com/Mission-Analysis-Design-Technology-Library/dp/1881883108) For more

###### 4) Antenna Design in Small Satellites for Inter-Satellite Links:
- "MIMO antennas might not be the best option for inter-satellite communication in small satellites due to the characteristics of the propagation channel between the satellites."
- "There are two antenna techniques than can be used for inter-satellite communications: broad beam width isolated antennas and antenna arrays"
- "A recent experiment for inter-satellite communications is __GAMANET__, intended to create a large ad-hoc network in space using ground stations and satellites as nodes with intersatellite links using S band frequency [58](http://esaconferencebureau.com/docs/default-source/14a04_docs/4s_programme_190514.pdf?sfvrsn=0)" _Uses complex antenna design. Can't locate any documentation just listings for the workshop presentation_.
- "According to link budget, a __maximum distance of 1000 km__ between satellites can be achieved using a 3 W transmit power." [58]
- "[59](http://utias-sfl.net/wp-content/uploads/SSC07-VI-2.pdf) (_[also see](http://digitalcommons.usu.edu/cgi/viewcontent.cgi?article=3167&context=smallsat)_) and [60](http://digitalcommons.usu.edu/cgi/viewcontent.cgi?article=1996&context=smallsat), antenna designs proposed for inter-satellite links for nanosatellites and cubesats are mostly S-band single-patch antennas"
- Authors discuss the relative specifications requirements that can be imposed by multiple factors, most of which is not relevant in this case but provides a __strong structure for stating assumptions__
- "The __minimum bandwidth__ of the inter-satellite link is generally 1 MHz"
- ISLs are typically __full-duplex__ on a single antenna: 
    + "Frequency Division Duplex (__FDD__) system, transmission and reception bands can be separated using diplexor or circulator"
    + "Time Division Duplex (__TDD__) architectures, transmission and reception paths are separated using an RF switch controlled by the timing signals of the communication systems"
- For antenna the choice is multiple omni antenna or fewer antenna arrays with beam steering. Need to consider design aspect of surface area for solar cells. Authors provide a compare and contrast for the two approaches. 
- ISLs for CubeSats cited [63](http://ieeexplore.ieee.org/document/6496947/?arnumber=6496947)

##### B. Link Layer

- Framing, Medium Access Control, MAC addressing, synchronization, error control, flow control, and multiple access _all that good stuff in one layer_
- Factors which inform MAC protocol performance:
    + Energy efficiency: The energy consumed per unit of successful transmission
    + Scalability and adaptability
    + Channel utilization
    + Latency & Throughput
    + Fairness
- Contention based: ALOHA, CSMA, BTMA, ISMA 
- Conflict-free: TDMA, FDMA, CDMA, OFDM, SDMA
- Nothing mentioned up to this point that is space specific

###### [70](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=4584281)
- "proposes IEEE 802.11 physical and MAC layers for space based Wireless Local Area Network (WLAN)"
- Working on optimizing inter-frame spacings (SIFS, Distributed Co-ordination Function (DFC) IFS, Point Co-ordination Function (PFC) IFS, Extended IFS)
- Tested work against two separate scenarios __"triangular and the circular flower constellations"__ w/ an elected master acting as an access point
- "In LEO networks ... propagation delays are in the order of milliseconds"
- IFSes adjusted based on distance correlated to some propagation delay _Could this be done collaboratively without massive overheads/thrashing?_
- __Uses OPNET for simulation__
- "Finding optimum probability ratio between the collided packets and the successful packets, and relating the success ratio to the optimal contention window (CW), the satellites can adjust their CW minimum values adaptively thereby operating at optimal conditions." _hmm dubious_
- "It is concluded that the IEEE 802.11 can be extended to longer inter-satellite link with minimum degradation of throughput." _Could this be a starting point?_

###### [73 text book](http://www.igi-global.com/article/inter-satellite-communications-for-small-satellite-systems/93607) and [74 Page 238](http://design-development-research.co.za/wp-content/uploads/2015/04/DDR2014Full-Proceedings.pdf)
- Same author as this survey, Radhakrishnan. 74 is definitely worth a closer look
- "Carrier Sense Multiple Access with Request-to-Send and Clear-to-Send protocol (CSMA/CA/RTS/CTS) is proposed" 
- TS packets can be sent through directional means
- Sat formation will obviously determine how much overhearing is possible, hidden node problems are likely. _could collisions accurately be detected?_
- "It is concluded that the proposed MAC protocol is suitable for missions that do not require tight communication links."

###### QB-50 [75](http://ieeexplore.ieee.org/document/5640977/)
- _already have [notes on this paper](https://github.com/StarStuffSteve/masters-research-project/blob/master/Publication%20Notes/Preliminary%20internetworking%20simulation%20of%20the%20QB50%20cubesat%20constellation%20-%202010.md) which I personally think is kinda bunk_
- They do some TCP v. UDP simulation
- "The proposed multiple access protocol is AX.25"

###### Code Division Multiple Access (CDMA) for Precision Formation Flying (PFF) [78](http://repository.tudelft.nl/islandora/object/uuid:0f15a445-9e2d-4468-afcc-08a1d8a09c55/?collection=research) and [79](http://repository.tudelft.nl/islandora/object/uuid:d061e0c8-99cd-419e-be44-225fe063f84c/?collection=research).
- Same authors for both. Delft.
- "The system evolves in to a centralized graph with one spacecraft chosen to be the reference for a particular time period and subsequently enabling various science missions, for example, multi-point remote sensing"
- "__half-duplex CDMA__ is selected as a suitable network architecture since it enables both code and carrier phase measurements, and also supports reconfigurability and scalability within space based sensor networks"
- "The Multiple Access Interference, along with Doppler effects and near far problem, worsens navigational accuracy which is a critical issue in precision formation flying missions"
- "When the satellites are in close proximity, an adaptive power control mechanism, lowering the power of the transmitted signals, can be used to minimize the effect of near-far problem"

###### Load Division Multiple Access (LDMA) is investigated in [80](http://ieeexplore.ieee.org/abstract/document/6104558/)
- Competition based mode switching between Low Contention Level - CSMA and HCL - TDMA
- Nodes given priority within LCL - CSMA mode 
- No RTS/CTS
- Switching between modes based on number of __conflict frames__ received
- Master satellite collecting data for downlink in simulation scenario
- OMNET++ & STK utilized
- 72% channel utilization with LDMA over CSMA(44%) and TDMA(61%)
- Benefits reduce as number of nodes increase, will begin to approach pure TDMA

###### T/CDMA Hybrid [13](https://www.researchgate.net/profile/Fatemeh_Afghah2/publication/271214211_OPTIMAL_MULTIPLE_ACCESS_PROTOCOL_FOR_INTER-SATELLITE_COMMUNICATION_IN_SMALL_SATELLITE_SYSTEM/links/54c295b80cf256ed5a8ee1bd.pdf)
- Same author as this survey, Radhakrishnan.
- Network divided into mastered clusters
- Master re-clustering performed using closeness centrality algorithm
- "The hybrid TDMA/CDMA protocol has high throughput and delay as compared to other protocols"

###### FDMA/TDMA Hybrid [81](http://ieeexplore.ieee.org/document/6737551/?arnumber=6737551&tag=1) 
- Design by modifying the MAC and PHY layers of WiMedia
- 2D Time/Frequency slots (super frames)
- Multiple modes of operation proposed based on state of network _seems dangerously complex and likely to experience frequent switching and perhaps non-convergent thrashing_
- Perhaps a good example of a dubious non-generalized approach

###### Standards
- The Consultative Committee for Space Data Systems (CCSDS) protocol standards have not been used in LEO small sat missions as most are more suitable for deep space DTN style applications
- Another handy dandy table on 2456

##### C. Network Layer

- "The choice of the routing scheme is also dependent upon the mission requirements, whether it is possible to use a completely distributed or centralized system."
- Border Gateway Protocol-Satellite version (BGP-S) [85](http://ieeexplore.ieee.org/document/966264/?arnumber=966264&tag=1)
- Distributed multicast routing scheme [86](http://dl.acm.org/citation.cfm?id=942555)
    + "The authors proposed a modification to the Multi-Layered Satellite Routing (MLSR) algorithm by adapting the algorithm to handle mobility of the satellites."

###### Protocols for Route Discovery for a Network of Small Satellites [87](http://www.sciencedirect.com/science/article/pii/S1389128604002221)
- "Neighbor Discovery Protocol (focus), Network Synchronization Protocol, Decentralized Routing Protocol, Node Affiliation Protocol and Packet forwarding Protocol"
- Node neighbors are either 'new' (node has no insight) or 'recurring' (node has complete insight)
- Node transmits a HELLO message (omni-directional) the response to which includes codes and sync information. 
- Neighbor discovery ends with exchange of orbital parameters
- Details are sparse will have to read 87

###### Discussion of Routing Algorithms for LEO Satellite Systems [88](https://www.ijircce.com/upload/2014/acce14/22_P199Satellite.pdf)
- Handover optimization 
- Proposes a connection matrix which describes connection state of sats in network
- Perhaps worth a look for the purpose of reference

###### Bandwidth Delay Satellite Routing (BDSR) [89](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=6040248)
- Path taken is either the path of maximum bandwidth or the path of minimum delay - _Need to investigate how the algorithm makes predictions_
- 6 orbital planes each containing 11 sats
- BDSR algorithm reduces to __shortest path when bandwidth is fixed__
- Optimizing for bandwidth always increases delay by a "large margin"

###### Dynamic Routing Algorithm Based on MANETs [93](http://ieeexplore.ieee.org/document/1554029/)
- Examined scenario diverges from our case
- Cluster based
- Makes "the assumption that intra-cluster satellite topology is known."
- "Satellites in the network know to which cluster other satellites belong"
- All nodes can calculate the path to any other cluster
- Relative locations of clusters is known by nodes
- "They emphasized Asynchronous Transfer Mode (ATM) based routing schemes for a network of small satellites by preparing virtual topology using virtual connections between the satellites." - _will have to read further_
- "The proposed routing algorithm utilizes the advantages of both static and dynamic routing" (ZRP)

###### Other Publications/Topics Mentioned
- Destruction Resistant Routing Algorithm (cluster based) [90](http://ieeexplore.ieee.org/document/1357248/)
- Steiner tree [91](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=4384138)
- Distributed multipath routing [92](http://www2.tku.edu.tw/~tkjse/14-2/09-AE9901.pdf)
- A few paragraphs are dedicated to DTN but this is out of scope in our case [96](http://article.sapub.org/10.5923.j.jwnc.20130303.01.html)

### Proposed ISC Solutions

##### A. SDR Solutions to Small Satellite Challenges
- __SDR is the current state of the art__
- Authors give introduction of CR and SDR including examples of satellite focused SDRs
- Overview of GNU radio and USRP
- "Software defined radio inter satellite links can provide relative position, time, and frequency synchronization for small satellites."
- "An SDR inter-satellite link will be able to create automatically an ad-hoc inter-satellite link between the satellites and ground link capabilities" - ... _this seems like a stretch?_
- Authors note considerable benefits to SDRs
- "The most widely used software architecture for SDR is the Software Communications Architecture"
- Might be provide a reasonable introduction [108](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=6880174)
- "Operationally, it is difficult to rely on a spacecraft’s attitude control system to maintain antenna pointing for a fixed beam antenna. Using an array of low directivity elements and steering of the beam electronically proves to be a better solution."
- "The goal for SDR is to move the digital domain (modulation/ demodulation, encoding/decoding) as close as possible to the antenna"
- _Must further consider the effects of SDR use on the link layer..._
- Multiple examples of existing small satellite SDRs provided __Good reference material__
- "Using Software Defined Radio technology, we designed and implemented an optimal inter-satellite communications for a distributed wireless sensor network of small satellites [14](http://ieeexplore.ieee.org/document/7392985/?section=abstract)." - _reminder: read this paper_

###### Modular Antenna Array for Cubesats in Formation Flying Missions
- _skimmed this section_

##### C. Optimum MAC Protocols for Inter-Satellite Communication for Small Satellite Systems

###### 1) Modified CSMA/CA/RTS/CTS Protocol [74](http://design-development-research.co.za/wp-content/uploads/2015/04/DDR2014Full-Proceedings.pdf)
- _Definitely should give 74 a close read_
- "based upon distributed coordination function, one of the services offered by the IEEE 802.11"
- Leader-follower, cluster and constellation formations considered
- Reactive routing protocol
- "__assume__ that the satellites are deployed in nearly circular lower Earth orbits"
- "__assumed__ that all satellites share the same transmission frequency."
- __Table on 2464__ is a great guide for base mission parameter assumptions 
- _they used the phrase "extensive simulations" nine times_
- "The system performance was analyzed based on three different parameters, average end-to-end delay, average access delay, and throughput"
- "The leader-follower and constellation configurations have more throughput in comparison to cluster" - _is this loss worth the lost of ground cover that comes from lf?_
- "maximum throughput that can be achieved by using the proposed protocol for leader-follower and constellation formation flying pattern is around 24%, and for cluster configuration is around 11%."
- "the proposed protocol is suitable only for missions that can tolerate communication delays" - _suitable in our scenario, throughput is the concern not latency_

###### 2) Hybrid TDMA/CDMA Protocol
- "TDMA allows collision free transmission and DS-CDMA offers simultaneous transmission and better noise and anti-jam performance."
- "satellite network can be divided into clusters with each cluster having a master satellite and several slave satellites"
- "The slave satellites within a cluster communicates with the master satellite, and the master satellite forwards the data to the destination" - _master rotation and re-clustering obviously necessary_
- "propose to use closeness centrality algorithm for the selection of master satellite which satisfies the minimum power requirement (threshold, Pth)."
- Two approaches: TDMA centric and CDMA centric.
    + TDMA: each cluster is assigned a unique code, each node provided slots for comms w/ master of cluster, inter-cluster comms can occur in parallel due to coding
    + The TDMA centric hybrid protocol can be used in missions where the packet size varies considerably - Adaptive TDMA could be used
    + CDMA: each node has unique code, nodes can communicate in parallel to master, inter-cluster comms performed on schedule
    + The CDMA centric system can be used when the packet size is relatively consistent - _our case_
    + _must investigate cluster master selection, the assumption seems to be that the master has suitable battery and is capable of communications with other masters_
- Leader-follower, cluster and constellation formations considered
- "__assume__ that each satellite cannot generate a new message until all packets of the current message are transmitted, __and__ data packets generated in the current frame have to wait for the next frame for transmission."
- "The __average end-to-end delay__ is almost constant for the hybrid TDMA/CDMA protocol, but it increases for the CSMA/CA/RTS/CTS protocol for increasing traffic"
- __"TDMA/CDMA protocol has a higher throughput of 95%"__ - __Damn!__ - _what work is left for me to do?_
- See table on 2467

### Design Parameters for ISC Design Process
- Network topology
- Frequency of data transmission which can be influenced by nature of data:
    + Science data
    + Navigation data
    + Sat health/status data
    + Command/Control data
- Bandwidth requirements
- Real-time access requirements
- Processing capabilities _we'll be assuming all sats are identical_
- Reconfigurability and scalability
- Variable connectivity
- Variable data size
- Missions types [15](https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/20100019266.pdf), [17](http://ieeexplore.ieee.org/document/5207326/?arnumber=5207326&tag=1), [20](http://digitalcommons.usu.edu/cgi/viewcontent.cgi?article=1916&context=smallsat)
- "For missions demanding autonomous functionalities, small satellites would require high power, bandwidth, real time access to the channel, and processing capabilities"

### Future Research Directions
- "Cross-layer optimization for small satellites represents another research area to be investigated."
- "Future missions will demand autonomous transfer of data where today such transfers involve high levels of manual scheduling from Earth"
- "satellites should recognize all possible combinations of network topologies they may form and wisely decide a suitable one for communication" 

### Conclusions
- _General summary of topics covered_
- "This survey will serve as a valuable resource for understanding the current research contributions in the growing area of inter-satellite communications"