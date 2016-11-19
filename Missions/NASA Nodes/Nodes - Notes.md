# Nodes

#### Overview

- Identical craft to those of the EDSN Sats but with “enhanced software”
- “will test new network capabilities for operating swarms of spacecraft in the future.”
- Much more command and control oriented than EDSN
- [Tracking](http://nodes.engr.scu.edu/)
- Mission successfully demonstrated: indirect command, CubeSat science crosslink before DL and autonomous reconfiguration of the network setup to insure best DL
- “After two weeks it is expected that the satellites will be more than 60 miles (100 kilometers) apart, making it difficult for them to communicate with their __UHF radios__”

#### Technology and Capabilities

- EDSN: S-Band Downlink
- EDSN: UHF ISL
- EDSN: UHF Beacon (60s)
- Captain (CAP) and Luitenent (LT) roles. CAP collects, aggregates and downlinks
- CAP is dynamically chosen
- All comms are scheduled to insure max power saving
- Neat: “Six torque coils embedded in the solar panel PCB are used for attitude control”
- Very simple Ack/ReTX approach for comms
- “Custom protocol” - _Exact specification unavailable_
- Each session is a “transaction” which entails passing a command (Ack) and then receiving data
- CAP keeps special queue for commands destined for the LT

- Data sessions start with 12 pings (ID and Checksum) over 110 seconds
- 120s after getting a successful ping LT starts sending data (Starts w/ SOH) - _change from EDSN_
- __No Ack/Nack for data comms__
- “It is anticipated that future enhancements to the architecture will provide greater guarantees of data transmission either through __ACK/NACK of DTNs__”
- Queues and stacks are the sames EDSN (See EDSN docs)

- At start of each new CAP cycle CAP will request metrics from LT to compare to own, if better sends promote command
- LT with promote command will send a demote command to the CAP
- __3-4 comm sessions are scheduled per 25 hours__
- CAP is responsible for determining when it will be over the ground station
- __Clock effects could leave nodes 12s out of sync__ _They realign clocks at each scheduling session_
- All comms have buffers at beginning and end to account for this drift

#### Performance (14 days of data)

- 356/470 packets (_size uncertain_)
- Node K: 145/180
- Node J: 211/290
- 5 Successful captaincy negotiations
- Asym links lead to multiple transmissions of the same command as Acks were lost
- Last successful crosslink was 7 days into mission at __100km__
- A total of 12 crosslink successful crosslink sessions took place

