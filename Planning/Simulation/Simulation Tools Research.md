# Simulation Tools Research

## Requirements
- Model movement of nodes
    + Preferably: Three dimensional dynamics obtained from parsing data modelled on Tianwang and Nodes missions using [STK](https://www.agi.com/products/stk/)
    + At least: The ability to configure communications windows
- Model node power consumption
    + Preferably: Configurable to the point of modelling both compute and communications power consumption as well power repleneration from solar cell charging
    + At least: The mainanence of power levels for each node
- Communications protocols
    + Preferably: Use of elements from the X.25 protocol suite
    + At least: 'Straighforward' configuration of the TCP/IP stack
- Performance analysis
    + _See 'key metrics' [link](https://github.com/StarStuffSteve/masters-research-project/blob/master/Proposal/Revised%20Research%20Proposal.md)_

- Nice-to-have: Model error rates as a function of inter-node range
- Nice-to-have: Logical grouping of nodes with seperate configurations for each group (orbits)

## Non-requirements
- Model the physical layer protocol
- Hardware in the loop
- ...

## Potential Tools 

#### [ns-2](http://www.isi.edu/nsnam/ns/)
- Can be used to measure ["WSN power consumption"](http://www.mdpi.com/1424-8220/13/3/3473/pdf)
- C++
- 'Energy Models' supported and popular
- 3D motion possible but not looking easy
- No X.25 support
- CLI based

#### [ns-3](https://www.nsnam.org/)
- Considerably improved documentation
- C++/Python
- Strengthen mobility support that is inheritly 3D
- Same energy modelling features as ns-2
- No X.25 support

#### [SNS3 (ns-3 extension)](http://satellite-ns3.com/)
- Commercial _contacted them_
- Developed in conjunction with ESA
- Direct extension of ns-3 and thus has all ns-3 features
- Additional relevant error models

#### [Alanax (ns-3 STK integration)](http://www.alanax.com/#our-software)_
- Create scenarios in STK which can be simulated directly in ns-3
- Difficult to know exact specifications
- In beta 
- If I agree to help test and possibly contribute I have free usage [Brian Barritt]

#### opnet 
- Commercial
- 2D motion using predefined trajectories and vectors, periodic motion may be decidedly difficult
- Can be used to measure ["WSN power consumption"](http://www.mdpi.com/1424-8220/13/3/3473/pdf)
- Can be configured to use X.25 protocol suite

#### NetSim
- Seems to have been a version with X.25 support
- Power modelling possible
- Looks like there is less inherent mbility support than ns-3
- Popular for WSNs and MANETs it seems

#### omnet++
- Much more rudimentary
- Module/Channel structure (C++)
- More a tool for building network simulators than a simulator in and of itself?
- [Castalia](https://castalia.forge.nicta.com.au/index.php/en/)
- [INETMANET](https://github.com/aarizaq/inetmanet-3.x)





