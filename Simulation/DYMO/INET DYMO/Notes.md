### Dynamic MANET On-demand (AODVv2) Routing

- Slaves as clients? Check if INET implementation is Client compatible.

- AODVv2 is closely related to AODV [RFC3561], and has some of the features of DSR [RFC4728]

- During route discovery, an AODVv2 router multicasts a Route Request message (RREQ) to find a route toward a particular destination
- Using a hop-by hop retransmission algorithm, each intermediate AODVv2 router receiving the RREQ message records a route toward the originator
- Each __intermediate__ AODVv2 router that receives the RREP creates a route toward the target, and unicasts the RREP hop-by-hop toward the originator.
- When RREP received all intermediate nodes should also have a route

#### Debugging
- The RREQ message contains routing information to enable RREQ recipients to route packets __back__ to OrigNode
- the RREP message contains routing information enabling RREP recipients to route packets to TargNode
- The coordination among multiple AODVv2 routers to distribute routing information correctly for a shared address (i.e. an address that is advertised and can be reached via multiple AODVv2 routers) is not described in this document _clients only?_
- RREP not received within RREQ_WAIT_TIME, RREQ_Gen may retry the Route Discovery by generating another RREQ _Tested, some effect_
- After the attempted Route Discovery has failed, RREQ_Gen MUST wait at least RREQ_HOLDDOWN_TIME before attempting another Route Discovery to the same destination. _shouldn't fail_
- __Data packets awaiting a route SHOULD be buffered by RREQ_Gen__. This buffer SHOULD have a fixed limited size (BUFFER_SIZE_PACKETS or BUFFER_SIZE_BYTES). _Is this the reality in INET?_

#### Multi-NIC
- When multiple interfaces are available, a node transmitting a multicast packet with IP.DestinationAddress set to LL-MANET-Routers SHOULD __send the packet on all interfaces that have been configured for AODVv2 operation.__

#### Route Metrics
- Route selection in AODVv2 MANETs depends upon associating metric information with each route table entry.
- The most significant change when enabling use of alternate metrics is to require the possibility of multiple routes to the same destination, where the "cost" of each of the multiple routes is measured by a different alternate metric. __?__
- AODVv2 must also be able to invoke an abstract routine which in this document is called "LoopFree(R1, R2)". LoopFree(R1, R2) returns TRUE when, given that R2 is loop-free and Cost(R2) is the cost of route R2, Cost(R1) is known to guarantee loop freedom of the route R1. _English ..._
- For routes R1 and R2 using Metric Type 3 (Hop Count) [RFC6551], LoopFree (R1, R2) is TRUE when Cost(R2) <= (Cost(R1) + 1)
- Whenever an AODV router receives metric information in an incoming message, __the value of the metric is as measured by the transmitting router__
- AODVv2 does not store routes that cost more than MAX_METRIC[i]

#### Multicast
- Most AODVv2 messages are sent with the IP destination address set to the linklocal multicast address LL-MANET-Routers [RFC5498] unless otherwise specified.
- retransmitting multicast packets in MANETs SHOULD be done according to methods specified in [RFC6621].

#### Route States
- During use, an Active route is maintained continuously by AODVv2 and is considered to remain active as long as it is used at least once during every ACTIVE_INTERVAL
- When a route is no longer Active, it becomes an Idle route
- After a route remains Idle for MAX_IDLETIME, it becomes an Expired route; after that, the route is not used for forwarding

#### SEQ
- When a data packet is received for forwarding and there is no valid route for the destination, then the AODVv2 router of the source of the packet is notified via a Route Error (RERR) message
- AODVv2 uses sequence numbers to assure loop freedom [Perkins99], similarly to AODV
- The value zero (0) is reserved to indicate that the SeqNum for a destination address is unknown.
- MAX_SEQNUM_LIFETIME is the time after a reboot during which an AODVv2 router MUST NOT transmit any routing messages

#### Router Client
- An AODVv2 router may be configured with __a list of other IP addresses and networks which correspond to other non-router nodes__ which require the services of the AODVv2 router for route discovery and maintenance. An AODVv2 is always its own client, so that __the list of client IP addresses is never empty__.

#### Multi-NIC
- AODVv2 supports routers with multiple interfaces, as long as each interface has its own IP address
-
