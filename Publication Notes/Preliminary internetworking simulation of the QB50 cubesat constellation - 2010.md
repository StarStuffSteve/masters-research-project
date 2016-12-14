# QB50 inter-networking notes

### Premise
- TCP/UDP over IP
- e2e delay and throughput "to assess throughput and delay parameters as a function of link bandwidth intersatelital"

- Benefits of CubeSat networks discussed briefly
- QB50 mission detailed

### "TCP/UDP OVER CONSTELLATIONS"
- TCP Slow start and fast recovery discussed
- TCP Westwood solutions discussed (Connection analysis based on ACK recieve rate)
- TCP Westwood e/ NewReno and UDP discussed in paper

### "SIMULATION ENVIRONMENT AND TOPOLOGY"
- (1) scenary ring of 50 equally spaces C/Ss
- (2) scenary ring of lengthn 10,000km _pictures included_
- __no inter-orbit__
- __used ns-2 w/ Lloyd Wood's NS-Sat-Plot__
- __used SaVi__
- polar plane 79 at 300km
- Nine ground stations
- Up/Downlink at 9.6kbps
- ISL rates: 0.5, 1, 3, 6, 8, 10kpbs
- Bit rate is considered constant
- packet size of 210B (UDP) or 1040(TCP) sent on one second intervals (timing/sync)
- Link layer "basic one for NS" no FEC (forward error correction) or ARQ (automatic repeat request)
- packet corruption probablity set at 0.1%
- Taildrop queue with a max of 50 packets
- Packets routed to groundstation (collection)
- Within which there are event driven communication times
- 1500s simulation time

### "SIMULATION RESULTS"
- (1) TCP no TP increase from 6 to 10kbps 
- (1) TCP total downlink of 8.4kbps achieved w/ 6kbps ISL being sufficent to achieve this rate
- (1) Authors poorly desribed ISL low rate scenarios 
- ... higher ISL speeds = better (not much surprise there)

### "CONCLUSIONS"
- TCP saturation limit imposed by downlink throughput
- UDP less delay but also more packet loss (better w/ error correction)
- "it must be concluded that ISL values should be selected depending on the traffic to be generated, otherwise long delays and low throughput values will be observed"
