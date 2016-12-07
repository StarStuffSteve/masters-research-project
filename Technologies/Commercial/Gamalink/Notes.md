# Gamalink
Generic sdr-bAsed Multifunctional spAce LINK _they're not even trying_

###### [Tekever: Project Description](http://gamalink.tekever.com/project-description)
###### [Cordis Final Report Summary](http://cordis.europa.eu/result/rcn/172006_en.html) _good summary of involved parties_
###### [Amateur Radio Report](http://www.arrl.org/news/more-chinese-amateur-radio-satellites-are-aloft)

- "SDR-based Ad hoc Space Networks (SASNETs), combining the flexibility of Software-Defined Radio with the self-configuration capability of ad hoc networks."
- "GNSS receiving, attitude determination, ranging, or clock synchronization without any additional hardware overhead."
- __Used in TW-1 missions__
- 2.4GHz spread spectrum

### Promotional Specs
_from [link](https://indico.esa.int/indico/event/133/contribution/14/material/0/)_
_e-mail sent to authors_

    Frequency range: 300 MHz to 3GHz
    Bandwidth: 40 MHz
    Data Rate: up to 2 Mbit/s
    Ranging precision: <50 cm @ 10Km
    GPS update rate: 5 Hz
    PCB size: 95.9 x 90.2 x 11 (mm)
    Total PCB mass: <100 g
    Data Interface: I2C, UART
    Storage capacity: from 2 x 2GB
    Supply Voltage: 3.3V or 5V

### From Single to Formation Flying CubeSats- An Update from the Delfi Programme
_e-mail sent to authors_

    Frequency: 2.45GHz
    Bandwidth: 40 MHz
    Positions precision: 5m
    PCD size: 80*80*10 mm^3
    PCB mass: <100g
    Number of antennas: 4 (3 S-band + 1 GPS)
    Data Interface: I2C, UART
    Supply Voltage: 3.3V
    Power consumption: <1.5 W (transmitting), <200mW (S-band receiving), <50mW (GPS receiving)

### The STU-2 CubeSat Mission and In-Orbit Test Results 
_post launch_
_e-mail sent to Wu_

- Dr. Shufan Wu
- STU-2 a.k.a TW-1
- __S-Band 2.456GHz @ 125kbps QPSK w/ <10^6 bit error rate__
- __Up/Downlink @4.8kbps 2-FSK in UHF (435-438 MHz)__
- __Battery: 2.6 Ah, 1 year__

### TW-1: A Cubesat constellation for space networking experiments
_pre-launch_

- _page 5 has nice graph_
- GOMspace supplying ADS-B (aircraft positioning)
- Mission task "__ISL : 60 kbps @ 600 km__"
- __using [Cubesat Space Protocol](https://en.wikipedia.org/wiki/Cubesat_Space_Protocol)__
- __static routing table programmed into the source-code of each sub-system__
- 4-2GB of storage

### Gamalink Project Final Report (2015)
- First three chapters provide an overview
- Constraints and requirements based on "CubeSat standard (size, mass, materials, available power...), the space environment (temperature, cosmic radiation and charged particles at different altitudes), and the baseline QB50 mission requirements (scientific, operational and launch)."
- Follwowed state of the art in MANETs
- "Afterwards, a summary of the extensive studies performed in routing protocols and metrics, resource management, load balancing and Quality of Service (QoS) were analysed, with some considerations on the GAMALINK application. The algorithms have been successfully implemented and tested in the SDR platform" _No more lower-level information provided in report_
- IP is split accross project consortium

### QB50 CubeSat Design Overview Report (i-INSPIRE II)

    Frequency: S-band (2.40-2.45 GHz)
    Bandwidth: 40 MHz
    Data Rate: up to 1 Mbit/s
    Protocol: __GAMANET__
    Positioning precision: 5 m (GPS)
    GPS update rate: 5 Hz
    GPS message format: SiRF-like binary (NMEA available on request)
    PCB size: 95.9 x 90.2 x 11 (mm)
    Total PCB mass: < 100 g
    Antennas: 3-axis stabilised Spinning: 4 (3 S-band + 1 GPS) 10 (6 S-band + 4 GPS)
    Data Interfaces: UART, SPI and I2C protocols.
    Storage Capacity: 2 x 2 GB
    Supply Voltage: 3.3V (5V also available)
    Average Power Consumption Transmitting/Receiving: < 1.0 - 1.5 W / < 200 - 500ms

- GAMAlink recorded as TRL5 _should be higher after TW mission_
- QB50 collaborating the the "GAMANET team"
- Furhter GAMAlink specs given for UHF/VHF up/downlink configuration
- __"For QB50, it is required that the satellite needs transmit beacon signals at least every 30 seconds."__
- Each beacon @ 100B takes <1s to transmit using 25% duty cycle for operating the tranmitter
- __Pages 38/39 contain large GAMAlink performance characteristics table__

### Robust & Flexible Command & Data Handling On-Board The Delfi Formation Flying Mission
- _masters thesis_
- "According to the latest information the data can be relayed over a maximum of four nodes/satellites [25]."
- __Future Work__ "it still needs to be proven how much of the relayed data over the GAMANET will eventually reach the Delfi ground station and telemetry database"
- ... TBC

### System Architecture Definition of the DelFFi Command and Data Handling Subsystem
- _masters thesis_
- ... TBC
