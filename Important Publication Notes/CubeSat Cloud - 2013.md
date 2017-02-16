# CubeSatCloud

###### 7.5 Network Configuration

"CubeSat to ground station communication link is modelled with data rate of 9600
bps, a delay of 2 ms with a jitter of 200 us following normal distribution. We modelled
the CubeSat Cluster communication links using the specifications of RelNAV. Data rate
is 1 Mbps, link communication delay of 0.1 ms. Packet loss rate was set at 0.3%, with
a 25% loss correlation in order to simulate packet burst loses. We used Hierarchical
Token Bucket (HTB) and tc networking tool on Linux to shape the network traffic to our
requirements."