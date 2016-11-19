# Basics Objectives

_using STK11 to model C/S dynamics_

- Use existing TW and Nodes data to create probability distribution for distance change between CubeSats for some time step -> sample distribution to simulate movement of CubeSats
    + Impose a maximum range for communications (100km)
    + Randomly sample distribution to simulate movement of CubeSats
    + Use samples to infer maximum duration of comms and comms distance 

- Calculate minimum feasable distance between sats in a single Sun-Sync Polar Oribtal (SSPO) (600-800km LEO)

- Calculate max oribtal inclination offsets for SSPOs
    + Should be determined by maximum communication range
    + Assume C/Ss begin in phase and range and but drift over time
    + Want to produce a file that show when C/Ss' in different orbits will be in comms range