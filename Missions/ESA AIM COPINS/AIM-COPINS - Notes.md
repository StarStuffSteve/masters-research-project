# AIM-COPINS Notes

#### E-mail
-----------
From: Ivan Lloro Boada [link](lloro@ice.csic.es)

- ISL established through S-Band radio ("ESA furnished item currently under development.")
- "such transceivers establish a distributed network (not centrally managed by the orbiter) as well as clock synchronisation and ranging capabilities" - _Gamanet/Gamalink?_
- Network includes orbiter, CubeSats and lander (MASCOT-2)

#### Planning and support documentation
---------------------------------------

###### SysNova Doc (Last pages are most interesting):
_Assessment/Spec of potential solutions w/o much technical data or specifics_   

- ISL intended for comms for "telecommand, housekeeping, payload telemetry data relay"
- 2x3U available form factors
- CubeSats intended to have 3 month lifetime at asteroid
- less than 200g transceiver + 2 antennae at 60g providing _omni-dir comms__
- __1W receieve and 3W transmit power consumption__
- __Full duplex__
- __Symmtrical data rate of 'up to' 1 Mbps__
- __Total mission data value of up to 1 Gbit__

###### AD2 Docs:
_Info on asteroids, mostly not relevant to CubeSats_

###### AD50 Doc:
- Mission objective is to "release" CubeSats, no further objectives outlines

###### RD2 Doc:
_Doc focuses more on mascot and mission scheduling; not a lot of COPINS_

###### AD1 Payload i/f:
- AIM S/C: Optel-D optical downlink system of _2.5 Gbps downlink rate capability_
- Also equipped w/ 1550 nm C-band
- __Peak COPINS ISL data rate - 1 kbit/s__ _Overridder by later planning documents_

###### RD1 Doc:
_long yet interesting read_

- Total mass of C/Ss [kg] (including 10% margin) 13.2
- Power required by C/Ss during cruise for battery charging 1W
- Data volume over lifetime (to be relayed to GS through AIM s/c) [Gbit] 2
- Required pointing accuracy [°] +- 0.25
- Required positioning knowledge of S/C [m] +- 50
- Max. / Min. operational temperature [C°] 30 / - 30
- Max. / Min. non-operational temperature [C°] 50 / - 35
--------------------------------------------------------
- See pages __130 - 136__

"Alternative" Technology :

- Proba-3 (Gamalink):
- Best estimation at present is 1.5kg (including 20% margin) redundant and 0.3kg non-redundant w/o casing (for lander)
- Preliminary Link Budget can be closed for __1Mbps (4MHz BW), 100km range and 1W RF__ and hemispherical antennas on both side. TBC
- No support of Lander localisation (radiotracking)
- FLYCON: Proof of concept for a dual Communication and Navigation system for formation flying based on commercial wireless standard (WiMax, WiFi…). TRL 4 by quarter 2 of 2015
-----

Further __Gamalink__ Detials:

- Power: 1W RX (single receiver), 4W TX+RX(1W RF, single receiver). All TBC
- Networking capability (multiple nodes communications, based on CDMA)
- Coarse range estimation (1-10 m TBC, based on RF base band processing/Kaman filtering TBC).
