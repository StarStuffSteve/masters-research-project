## Optimal multiple access protocol for inter-satellite communication in small satellite systems

### Introduction
- "In [10](http://ieeexplore.ieee.org/abstract/document/6104558/), a hybrid combination of contention and scheduled based protocol is investigated for inter-satellite communication" - _LDMA paper_
- "The Frequency Division Multiple Access (FDMA) is not an economic choice as it requires a larger bandwidth"
- "The TDMA protocol that we propose is adaptive rather than fixed time slots to each user thus utilizing the channel effectively"
- "We have investigated the combination of TDMA with Direct Sequence CDMA (DS-CDMA). The advantages of using DS-CDMA are better noise and anti jam performance ..."

### Overview of MAC protocols

#### TDMA
- "A TDMA system will work in either of two modes; 
    + TDMA/FDD (Frequency Division Duplexing) where uplink and downlink communication frequencies differ
    + TDMA/TDD (Time Division Duplexing) where both uplink and downlink use the same frequencies"
- "In TDMA, the time slot allocated to a user __does not depend__ on whether or not the user has any data to be transmitted"
- __"Extended Time Division Multiple Access (ETDMA)__ attempts to overcome this problem by allocating time, according to need"
    + "This scheme increases the efficiency significantly when the partners in a conversation do not speak over one another."
    + _must read into ETDMA further_
- "It is relatively less expensive to set up a TDMA base station compared to other protocols [13]" - text Book _Zheng J. and Jamalipour A., Wireless Sensor Networks; A Networking Perspective_
- TDMA reduces extensibility and requires strict timing synchronization

#### CDMA
- "In the (Direct-Sequence) DS-CDMA, the data signal is directly multiplied by the code signal and modulates the wideband carrier"
- "usually some form of the Phase Shift Keying is used, e.g., BPSK, QPSK, etc"
- "In Frequency Hopping CDMA (FH-CDMA) scheme, the carrier frequency is not constant and changes after each time interval T."
- "The DS-CDMA system occupies the whole bandwidth for transmission, whereas FH-CDMA uses only a small part of the bandwidth that differs in time."
- Two types of FH-CDMA:
    + Fast hopping: Multiple hops per bit
    + Slow hopping: More bits per hop
- "In TH-CDMA system, the data signal is transmitted in bursts at time intervals determined by the code assigned to the user"
    + Each user transmits on one of the N slots, depending on the code assigned to the user
- Hybrid systems of DS/FH/TH can "make use of the specific advantages of each of the modulation techniques" although with the penalty of increase transmitter receiver complexity

### Hybrid TDMA/CDMA Protocol
- "FORMOSAT-7/COSMIC-2 is a Taiwan/USA project, which is scheduled to launch in two phases in 2016 and 2018" [15](http://digitalcommons.usu.edu/smallsat/2013/all2013/83/)
- "The members of the clusters will send the data to their respective master satellite and the master satellite forwards the aggregated data to the destination through other master satellites" - _this seems inherently flawed_
- "it is necessary to recluster the network in order to select the master satellite with enough power for communication"
- Centrality algorithm for master selection with minimum power threshold

##### Centrality Algorithm
- [16](http://www.sciencedirect.com/science/article/pii/S0378873305000833)
- Four main measures of centrality
    + Degree centrality: Number of connections incident on a node
    + Closeness centrality: Average distance to all other nodes
    + Betweenness centrality: Number of times a node is on the shortest path between two others
    + Eigenvector centrality: Influence of a node in a network (?)
- Authors use closeness centrality algorithm where distance is determined by "communication signal power"
- No mention of trigger for reclustering

##### Proposed Work
- "The Walsh-Hadamard or Gold sequences can be used for generating perfectly orthogonal codes [17](http://link.springer.com/chapter/10.1007/978-3-540-92295-7_47)."
- Each cluster is assigned a set of codes
- Why use a hybrid TDMA/CDMA approach:
    + Approach can be tuned to fit mission requirements without large redesigns
    + "Integration of different traffic types using coded transmission."
    + TDMA does scale well but by clustering the number of TDMA members is reduced and nodes can be more easily added to the network
    + Use of adaptive TDMA is possible, but adds obvious overheads
    + "Time scheduling can be used to control interference between codes thus ensuring high quality of service"
- Pure CDMA can suffer from cross correlation and the near-far effect. 
    + Cross correlation: non-perfect orthogonal PRN code - _restrict cluster size to mitigate_
    + The near-far effect can be mitigated by appropriate power control mechanism.
- __The suggested protocol requires strict time synchronization which can be achieved by using GPS/GNSS__

###### Frame Structure
- TDMA Centric
    + "Each cluster is allocated a unique code and each satellite within a cluster has a dedicated slot"
    + Slave use uplink slots to communicate to the master and the master uses downlink slots
    + The masters have uplink/downlink slots for intercommunications
    + "The master satellite can receive the signals from neighboring master satellites in one downlink slot"
    + Diagram on _page 10_ is useful
- CDMA Centric
    + Each node has unique code
    + Masters have slots for receiving in-cluster, transmitting in-cluster, receiving inter-cluster and transmitting inter-cluster
    + Diagram on _page 10_ is useful
    + "Can be used if the packet size is relatively consistent ... for missions where it is required to (frequently?) broadcast some important information to the cluster members"

### Simulation Results and Discussions
- __Leader-Follower pattern used__
- __CDMA Centric Approach__
- _Table of simulation parameters on page 11_
    + TX pwr 500mW - 2W
    + 3 sats per cluster
    + 3 - 9 clusters
    + 2 possible neighboring clusters
    + S-band
    + __ISL range 10km__
    + 100ms slots and 0.6s frames (frames are cycles)
- Metrics:
    + TP: (total data transmission time)/(total simulation time)
    + Access delay: The amount of time a packet is queued
    + E2E: Src -> dst delay
- (__Restriction__) "A satellite cannot generate a new message until all packets of the current message are transmitted completely"
- (__Restriction__) "A satellite which has generated a message in the current frame cannot try
to access the data slots in the same frame" (?)
- Access/E2E delay results are effectively constant w.r.t. the number of clusters and the volume of traffic - _each packet has to wait at least one frame before transmission_
- _Their graphs poorly designed ..._
- "When the traffic increases, most of the slots in the frame will be utilized for data transmission and thus can have a throughput of almost 100%"
- "or the scenario with 27 satellites in orbit, the system reaches saturation quickly"

#### Comparison with CSMA/CA/RTS/CTS protocol [9](http://www.igi-global.com/article/inter-satellite-communications-for-small-satellite-systems/93607)
- "the average access delay and end-to-end delay is more for the CSMA/CA/RTS/CTS protocol because of network congestion at very high traffic"
- "hybrid TDMA/CDMA protocol has a higher throughput of 95% compared to the CSMA/CA/RTS/CTS protocol with a __throughput of 24%__ for λ = 0.7 packets/second"

### Conclusions & Future Work
- "Our future work is to evaluate the performance of the TDMA centric protocol."
- "TDMA/CDMA protocol can also be analyzed for various formation flying patterns like cluster and
constellation for small satellite systems"
- They plan to varying formation and communication design parameters in future