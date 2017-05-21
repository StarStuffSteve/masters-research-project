# TCD MAI Computer Engineering: Research Project

Project completed as part of a taught masters in computer engineering at the [Trinity College Dublin](tcd.ie)

__Supervisor__: [Dr. Johnathan Dukes](https://www.scss.tcd.ie/Jonathan.Dukes/)

### Overview:
The project seeks investigate potential approaches to balancing power consumption with data throughput within CubeSat networks. The end goal of the world is shed light on methods which may be employed in future missions to maximize science data collection and mission duration. To this end a number of simulations of customized protocols are carried out with OMNeT++.

<p align="center">
  <img src="https://github.com/StarStuffSteve/masters-research-project/blob/master/Simulation/Visualized%20Pass.gif" />
</p>

A snippet from this work's simulations. I've also put up this short [YouTube video](https://www.youtube.com/watch?v=74j9mB3edAA) explaining some of this work's findings.

Please don't hesitate to contact [me](mailto:stennis@tcd.ie) with any queries you may have

---

### Abstract

CubeSats are small satellite platforms which have significantly reduced the cost of access to low Earth orbit over the past decade. Recent CubeSat missions have demonstrated the platform’s ability to form in-orbit networks. CubeSat Network (CSN) missions enable low-cost applications in coordinated sensing and low-bandwidth communications.

This work addresses a trade-off unique to CSNs. CubeSat satellite-to-ground (S2G) communication requires high levels of energy consumption to achieve data rates in the order of kilobytes per second. In comparison, CubeSats are capable of more energy efficient satellite-to-satellite (S2S) communication at rates an order of magnitude above those of S2G communication. This asymmetry underpins this work’s trade-off of interest, that of CSN power use against S2G data throughput.

Relevant areas of prior art are examined and specialized Medium Access Control (MAC) and routing protocols are proposed. This work’s proposed protocols are developed alongside a simulation of a hypothetical CSN mission using the open-source network simulator, OMNeT++. Proposed MAC protocol energy saving features are shown to decrease CSN energy consumption without a reduction in S2G throughput. This work’s proposed routing protocol introduces the energy sensitive election of a CubeSat dedicated to performing S2G communication. This election approach is shown to reduce the energy consumption of previously “over-worked” CubeSats. Additional adjustments to route discovery behaviour are required to ensure this approach does not reduce the overall energy efficiency of S2G communication.

---
