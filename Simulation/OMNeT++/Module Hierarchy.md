## Host
INetworkNode
    NodeBase
        StandardHost
            WirelessHost
                AODVRouter
                    *Mobility
                    *NetworkLayer
                    *RoutingTable
                    InterfaceTable
                    WirelessNIC
                    AODVRouting _many parameters_
                    IdealChannel

## Application
_Attached to Host_
IUDPApp __Used by StandardHost__
        UDPBasicApp
        UDPSink

## Networking
_Attached to Host_
INetworkLayer __Used by NodeBase__
    IPv4NetworkLayer

IRoutingTable __Used by NodeBase__
    IPv4RoutingTable

INetworkConfigurator
    NetworkConfiguratorBase
        IPv4NetworkConfigurator _Configurates_

## NIC
_Attached to Host_
INic
    IWirelessNic __Used by NodeBase__
        WirelessNic _Connects Network Layer to Radio via LMAC_

## MAC
_Attached to NIC_
LayeredProtocolBase
    MACProtocolBase
    IMACProtocol __Used by WirelessNic__
        LMacLayer

## Radio
_Attached to NIC_
#### Ideal Radio
_Use: IdealRadioMedium_
IRadio __Used by WirelessNic__
    Radio
        IdealRadio
            Radio -> ITransmitter -> IdealTransmitter
            Radio -> IReciever -> IdealReciever

#### APSK Radio
_Use: APSKScalarRadioMedium_
Radio
    NarrowbandRadioBase
        FlatRadioBase
            APSKRadio
                APSKScalarRadio:

APSKScalarRadio: Antenna
Radio
    IAntenna
        AntennaBase
            ConstantGainAntenna

APSKScalarRadio: Tr, Rc
Radio
    ITransmitter
        NarrowbandTransmitterBase
            APSKScalarTransmitter
    IReceiver
        NarrowbandReceiverBase
            APSKScalarReceiver

APSKScalarRadio: Energy
Radio
    IEnergyConsumer
        EnergyConsumerBase
            StateBasedEnergyConsumer
