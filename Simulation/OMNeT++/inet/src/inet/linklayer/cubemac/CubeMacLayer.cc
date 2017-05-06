#include "inet/linklayer/cubemac/CubeMacLayer.h"
#include "inet/linklayer/cubemac/CubeMacFrame_m.h"

#include "inet/common/INETUtils.h"
#include "inet/common/INETMath.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/FindModule.h"

#include "inet/linklayer/common/SimpleLinkLayerControlInfo.h"

#include "inet/networklayer/common/InterfaceEntry.h"

namespace inet {

Define_Module(CubeMacLayer)

// Might use again in future
//#define myId    (getParentModule()->getParentModule()->getIndex())

const MACAddress CubeMacLayer::CUBEMAC_BROADCAST = MACAddress::BROADCAST_ADDRESS;

void CubeMacLayer::initialize(int stage)
{
    MACProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        //
        // Added
        //
        startTime = par("startTime").doubleValue(); // Used to synchronize nodes

        isSlave = par("isSlave");

        isGround = par("isGround");

        myClusterId = par("clusterId");

        slavesInCluster = par("slavesInCluster");
        expectedSlaveDataPackets = slavesInCluster; // Reset during each sleep state

        timeoutDuration = par("timeoutDuration").doubleValue();

        slotPadding = par("slotPadding").doubleValue();

        currentSlotEndTime = 0.0;

        canSendNextPacket = false;

        pureTDMA = par("pureTDMA");

        energySavingFeatures = par("energySavingFeatures");

        // --- Results =, Stats etc.
        packetsOnQueue = 0;

        // ---

        queueLength = par("queueLength");

        slotDuration = par("slotDuration");

        bitrate = par("bitrate");

        headerLength = par("headerLength");
        EV << "headerLength is: " << headerLength << endl;

        numSlots = par("numSlots");

        if (pureTDMA){
            numSlots -= 1;
            uplinkSlot = -1;
        }
        else
            uplinkSlot = (numSlots - 1);

        EV_DETAIL << "My Mac address is" << myAddress
                  << " my cluster ID is " << myClusterId << endl;

        macState = INIT;

        initializeMACAddress();

        // Comes from MACProtocolBase
        registerInterface();

        // Important: Signal subscriptions for receiveSignal function
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);

        // Results, Stats, Watches etc.

        accessDelayMAC.setName("MAC Access Delay");

        WATCH(numSlots);
        WATCH(uplinkSlot);
        WATCH(packetsOnQueue);
        WATCH(macState);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        // Why commented?
        //int channel;
        //channel = hasPar("defaultChannel") ? par("defaultChannel") : 0;

        EV_DETAIL << " startTime = " << startTime
                  << " isSlave = " << isSlave
                  << " myClusterId = " << myClusterId
                  << " slavesInCluster = " << slavesInCluster
                  << " queueLength = " << queueLength
                  << " slotDuration = " << slotDuration
                  << " slotPadding = " << slotPadding
                  << " timeoutDuration = " << timeoutDuration
                  << " numSlots = " << numSlots
                  << " bitrate = " << bitrate << endl;

        // Creating initial self messages
        startCubemac = new cMessage("startCubemac");
        startCubemac->setKind(CUBEMAC_START_CUBEMAC);

        timeout = new cMessage("timeout");
        timeout->setKind(CUBEMAC_TIMEOUT);

        wakeUp = new cMessage("wakeUp");
        wakeUp->setKind(CUBEMAC_WAKEUP);

        sendData = new cMessage("sendData");
        sendData->setKind(CUBEMAC_SEND_DATA);

        scheduleAt(startTime, startCubemac);
    }
}

CubeMacLayer::~CubeMacLayer()
{
    // TODO: Add vectors
//    delete slotChange;

    cancelAndDelete(startCubemac);
    cancelAndDelete(timeout);
    cancelAndDelete(wakeUp);
    cancelAndDelete(sendData);

    // Clearing the queue
    for (auto & elem : macQueue) {
        delete (elem);
    }
    macQueue.clear();
}

// Same as IdealMac
void CubeMacLayer::initializeMACAddress()
{
    const char *addrstr = par("address");

    if (!strcmp(addrstr, "auto")) {
        myAddress = MACAddress::generateAutoAddress();

        // change module parameter from "auto" to concrete address
        par("address").setStringValue(myAddress.str().c_str());
    }
    else {
        myAddress.setAddress(addrstr);
    }
}

// TODO: Read more into Interfaces
// Same as IdealMac
InterfaceEntry *CubeMacLayer::createInterfaceEntry()
{
    InterfaceEntry *e = new InterfaceEntry(this);
    // Add param set/get methods here?
    // Who is using this i/f?
    e->setDatarate(bitrate);
    // generate a link-layer address to be used as interface token for IPv6
    e->setMACAddress(myAddress);
    e->setInterfaceToken(myAddress.formInterfaceIdentifier());
    e->setMtu(par("mtu").longValue());
    // e->setMulticast(false);
    e->setMulticast(true);
    e->setBroadcast(true);
    return e;
}

void CubeMacLayer::handleSelfMessage(cMessage *msg)
{
    if (isSlave)
    {
        if (pureTDMA)
            throw cRuntimeError("All nodes must be assigned the master role in pure TDMA mode");

        switch (macState) {
            case INIT:
                if (msg->getKind() == CUBEMAC_START_CUBEMAC) {
                    mySlot = uplinkSlot;

                    currSlot = -1; // Will be woken up first in slot 0

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    if (wakeUp->isScheduled())
                        cancelEvent(wakeUp);
                    scheduleAt(simTime() + slotDuration, wakeUp);
                    currentSlotEndTime = simTime() + slotDuration;

                    EV_DETAIL << "Slave: First wake up at: " << simTime() + slotDuration << endl;
                    EV_DETAIL << "Slave: Old: INIT, New: SLEEP" << endl;
                }
                else {
                    EV << "Slave: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }
                break;

            //---------------//

            case SLEEP:
                if (msg->getKind() == CUBEMAC_WAKEUP) {
                    currSlot++;
                    currSlot %= numSlots; // 0 .. uplinkSlot

                    EV_DETAIL << "Slave: In slot: " << currSlot << " My slot: " << mySlot << endl;

                    // ---> SEND_DATA
                    if (currSlot == uplinkSlot) {
                        EV_DETAIL << "Slave: Woken during uplink slot.\n";

                        macState = SEND_DATA;
                        radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

                        EV_DETAIL << "Slave: Old: SLEEP, New: SEND_DATA" << endl;
                    }
                    // ---> WAIT_MASTER_DATA
                    else {
                        EV_DETAIL << "Slave: Woken during non-uplink slot.\n";

                        macState = WAIT_MASTER_DATA;
                        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

                        // Wait for duration of timeout for a message
                        scheduleAt(simTime() + timeoutDuration, timeout);

                        EV_DETAIL << "Slave: Old: SLEEP, New: WAIT_MASTER_DATA" << endl;
                    }

                    scheduleAt(simTime() + slotDuration, wakeUp);
                    currentSlotEndTime = simTime() + slotDuration;
                }
                else {
                    EV << "Slave: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }

                break;

            //---------------//

            case WAIT_MASTER_DATA:
                /* Start Errors */
                if (msg->getKind() == CUBEMAC_SLAVE_DATA)
                    throw cRuntimeError("Slave: Expecting master data, received slave data.\n");
                /* End Errors */

                if (msg->getKind() == CUBEMAC_TIMEOUT) {
                    EV_DETAIL << "Slave: Data timeout. Going back to sleep.\n";

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Slave: Old: WAIT_MASTER_DATA, New: SLEEP" << endl;
                }

                else if (msg->getKind() == CUBEMAC_MASTER_DATA) {
                    if (timeout->isScheduled())
                        cancelEvent(timeout);

                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);

                    const MACAddress& dest = mac->getDestAddr();
                    const bool containsData = mac->getContainsData();
                    const bool isLastPacket = mac->getIsLastPacket();
                    const int clusterId = mac->getClusterId();

                    EV_DETAIL << "Slave: Received master data packet from src " << mac->getSrcAddr()
                              << " with dest " << dest
                              << " is last packet " << isLastPacket
                              << " cluster ID " << clusterId << endl;

                    if (!containsData) {
                        EV_DETAIL << "Slave: Data packet from master is empty" << endl;
                        delete mac;
                    }
                    else if (clusterId != myClusterId) {
                        EV_DETAIL << "Slave: Data packet from master from different cluster" << endl;
                        delete mac;
                        break;
                    }
                    else {
                        if (dest == myAddress || dest.isBroadcast() || dest.isMulticast() || dest == CUBEMAC_BROADCAST) {
                            EV_DETAIL << "Slave: Sending packet '" << mac << "' up\n";

                            sendUp(decapsMsg(mac));
                        }
                        else {
                            EV_DETAIL << "Slave: Data not broadcast or for me, deleting\n";
                            delete mac;
                        }
                    }

                    // Only sleep if the incoming message is marked as the last packet
                    if (isLastPacket && energySavingFeatures) {
                        macState = SLEEP;
                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                        EV_DETAIL << "Slave: Old: WAIT_MASTER_DATA, New: SLEEP" << endl;
                    }
                    else {
                        EV_DETAIL << "Slave: Received packet which is not the last that the sender intends to send" << endl;
                        EV_DETAIL << "Slave: Staying in WAIT_MASTER_DATA" << endl;
                        EV_DETAIL << "Slave: Resetting timeout" << endl;
                        scheduleAt(simTime() + timeoutDuration, timeout);
                    }
                }

                // Timeouts should probably make this impossible, something is wrong if we reach this
                else if (msg->getKind() == CUBEMAC_WAKEUP) {
                    if (timeout->isScheduled())
                        cancelEvent(timeout);

                    EV_DETAIL << "Slave: Waited for data until end of slot" << endl;

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Slave: Old: WAIT_MASTER_DATA, New: SLEEP" << endl;

                    if (wakeUp->isScheduled())
                        cancelEvent(wakeUp);
                    scheduleAt(simTime(), wakeUp);
                }
                else {
                    EV << "Slave: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }

                break;

            //---------------//

            case SEND_DATA:

                // Scheduled by receiveSignal() on radio mode change
                if (msg->getKind() == CUBEMAC_SEND_DATA) {
                    if (macQueue.size() > 0) {

                        // Copy next packet on queue
                        CubeMacFrame *data = macQueue.front()->dup();

                        delete macQueue.front(); // Frees pointer and deletes
                        macQueue.pop_front(); // Moves to new pointer?

                        data->setContainsData(true);
                        data->setKind(CUBEMAC_SLAVE_DATA);
                        data->setClusterId(myClusterId);

                        data->setIsLastPacket(true);
                        canSendNextPacket = false; // There must be a next packet and enough time to send it for this to true

                        // Current packet
                        double bitLength = (double) data->getBitLength();
                        double estTimeToSend = bitLength/bitrate;

                        if ((simTime() + estTimeToSend) >= (currentSlotEndTime + slotPadding)) {
                            throw cRuntimeError("Master: ERROR: Cannot send current packet before end of slot");
                        }

                        // Checking if we can send the next packet on our queue before the slot ends
                        if (macQueue.size() > 0) {
                            // Next packet
                            double nextBitLength = (double) macQueue.front()->getBitLength();
                            double nextEstTimeToSend = nextBitLength/bitrate;

                            simtime_t endNext = estTimeToSend + nextEstTimeToSend;

                            // If next
                            if ((simTime() + endNext) < (currentSlotEndTime + slotPadding)) {
                                EV << "Slave: Have calculated that there is time to send next packet on queue " << endl;

                                canSendNextPacket = true;
                                data->setIsLastPacket(false); // Let receiver know we are going to attempt to send another packet
                            }
                        }

                        const MACAddress& dest = data->getDestAddr();
                        EV << "Slave: Sending packet '" << data << "' to dest " << dest << endl;

                        simtime_t adm = simTime() - data->getArrivalTimeMac();
                        accessDelayMAC.record(adm);

                        sendDown(data);
                    }
                    // No data to send
                    else {
                        canSendNextPacket = false;

                        // Create new empty packet
                        CubeMacFrame *data = new CubeMacFrame();

                        data->setContainsData(false);
                        data->setKind(CUBEMAC_SLAVE_DATA);
                        data->setClusterId(myClusterId);

                        data->setIsLastPacket(true); // Empty packets are always last packets

                        data->setSrcAddr(myAddress);
                        data->setDestAddr(CUBEMAC_BROADCAST);

                        // ??? Isn't header just blank?
                        data->setBitLength(headerLength); // TODO: Need to make sure nothing is going wrong here

                        EV << "Slave: No data on queue. Sending empty broadcast packet"<< endl;

                        sendDown(data);
                    }
                }

                else if (msg->getKind() == CUBEMAC_WAKEUP) {
                    throw cRuntimeError("Slave: New slot starting while still transmitting data. Check data packet size and/or bitrate \n");
                }

                else {
                    EV << "Slave: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }

                break;

            default:
                throw cRuntimeError("Slave: Unknown MAC state: %d", macState);
                break;
        } // END SLAVE FSM
    }

    //---------------//
    //---------------//
    //---------------//

    else
    {
        switch (macState) {
            case INIT:
                if (msg->getKind() == CUBEMAC_START_CUBEMAC) {
                    mySlot = myClusterId; // Fixed time slot

                    currSlot = -1; // Will be woken up first in slot 0

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    scheduleAt(simTime() + slotDuration, wakeUp);
                    currentSlotEndTime = simTime() + slotDuration;

                    EV_DETAIL << "Master: First wake up at: " << simTime() + slotDuration << endl;
                    EV_DETAIL << "Master: Old: INIT, New: SLEEP" << endl;
                }
                else {
                    EV << "Master: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }
                break;

            //---------------//

            case SLEEP:

                // Used in wait slave data to count received slave packets
                expectedSlaveDataPackets = slavesInCluster;

                if (msg->getKind() == CUBEMAC_WAKEUP) {
                    currSlot++;
                    currSlot %= numSlots;

                    EV_DETAIL << "Master: In slot: " << currSlot << " My slot: " << mySlot << endl;

                    // ---> SEND_DATA
                    if (currSlot == mySlot) {
                        EV_DETAIL << "Master: Woken during my slot." << endl;

                        macState = SEND_DATA;
                        radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

                        EV_DETAIL << "Master: Old: SLEEP, New: SEND_DATA" << endl;
                    }
                    // ---> WAIT_SLAVE_DATA
                    else if (currSlot == uplinkSlot){
                        EV_DETAIL << "Master: Woken during uplink slot." << endl;
                        EV_DETAIL << "Master: expecting " << expectedSlaveDataPackets << " data packets from slaves " << endl;

                        macState = WAIT_SLAVE_DATA;
                        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

                        scheduleAt(simTime() + timeoutDuration, timeout);

                        EV_DETAIL << "Master: Old: SLEEP, New: WAIT_SLAVE_DATA" << endl;
                    }
                    // ---> WAIT_MASTER_DATA
                    else {
                        EV_DETAIL << "Master: Woken up in slot of another master" << endl;

                        macState = WAIT_MASTER_DATA;
                        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

                        scheduleAt(simTime() + timeoutDuration, timeout);

                        EV_DETAIL << "Master: Old: SLEEP, New: WAIT_MASTER_DATA" << endl;
                    }

                    scheduleAt(simTime() + slotDuration, wakeUp);
                    currentSlotEndTime = simTime() + slotDuration;
                }

                else {
                    EV << "Master: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }


                break;

            //---------------//

            case WAIT_MASTER_DATA:
                /* Start Errors */
                if (msg->getKind() == CUBEMAC_SLAVE_DATA)
                    throw cRuntimeError("Master: Expected master data received slave data\n");
                /* End Errors */

                if (msg->getKind() == CUBEMAC_TIMEOUT) {
                    EV_DETAIL << "Master: Master data timeout. Go back to sleep" << endl;

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Master: Old: WAIT_MASTER_DATA, New: SLEEP" << endl;
                }

                else if (msg->getKind() == CUBEMAC_MASTER_DATA) {
                    if (timeout->isScheduled())
                        cancelEvent(timeout);

                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);

                    const MACAddress& dest = mac->getDestAddr();
                    const bool containsData = mac->getContainsData();
                    const bool isLastPacket = mac->getIsLastPacket();
                    const int clusterId = mac->getClusterId();

                    EV_DETAIL << "Master: Received master data packet from src " << mac->getSrcAddr()
                              << " with dest " << dest
                              << " is last packet " << isLastPacket
                              << " with cluster ID " << clusterId<< endl;

                    if (!containsData) {
                        EV_DETAIL << "Master: Data packet from master is empty" << endl;
                        delete mac;
                    }
                    else {
                        if (dest == myAddress || dest.isBroadcast() || dest.isMulticast() || dest == CUBEMAC_BROADCAST) {
                            EV_DETAIL << "Master: Sending packet '" << mac << "' up" << endl;

                            sendUp(decapsMsg(mac));
                        }
                        else {
                            EV_DETAIL << "Master: Data not broadcast or for me, deleting" << endl;
                            delete mac;
                        }
                    }

                    // Only sleep if the incoming message is marked as the last packet
                    if (isLastPacket && energySavingFeatures) {
                        macState = SLEEP;
                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                        EV_DETAIL << "Master: Old: WAIT_MASTER_DATA, New: SLEEP" << endl;
                    }
                    else {
                        EV_DETAIL << "Master: Received packet which is not the last that the sender intends to send" << endl;
                        EV_DETAIL << "Master: Staying in WAIT_MASTER_DATA" << endl;
                        EV_DETAIL << "Master: Resetting timeout" << endl;

                        scheduleAt(simTime() + timeoutDuration, timeout);
                    }
                }

                // Timeouts should probably make this impossible, something is wrong if we reach this
                else if (msg->getKind() == CUBEMAC_WAKEUP) {
                    if (timeout->isScheduled())
                        cancelEvent(timeout);

                    EV_DETAIL << "Master: Waited for master data until end of slot" << endl;

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Master: Old: WAIT_MASTER_DATA, New: SLEEP" << endl;

                    if (wakeUp->isScheduled())
                        cancelEvent(wakeUp);
                    scheduleAt(simTime(), wakeUp);
                }

                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }

                break;

            //---------------//

            case WAIT_SLAVE_DATA:
                /* Start Errors */
                if (msg->getKind() == CUBEMAC_MASTER_DATA)
                    throw cRuntimeError("Master: Expected slave data received master data\n");
                /* End Errors */

                if (msg->getKind() == CUBEMAC_TIMEOUT) {
                    EV_DETAIL << "Master: Slave data timeout. Going back to sleep." << endl;
                    EV_DETAIL << "Master: Was expecting " << expectedSlaveDataPackets
                              << " more data packets from slaves in cluster." << endl;

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Master: Old state: WAIT_SLAVE_DATA, New state: SLEEP" << endl;
                }

                if (msg->getKind() == CUBEMAC_SLAVE_DATA) {
                    if (timeout->isScheduled())
                        cancelEvent(timeout);

                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);

                    const MACAddress& dest = mac->getDestAddr();
                    const int containsData = mac->getContainsData();
                    const bool isLastPacket = mac->getIsLastPacket();
                    const int clusterId = mac->getClusterId();

                    EV_DETAIL << "Master: Received slave data packet from src " << mac->getSrcAddr()
                              << " with dest " << dest
                              << " is last packet from this slave " << isLastPacket
                              << " with cluster ID " << clusterId << endl;

                    // Ignore data from outside of my cluster
                    if (clusterId != myClusterId) {
                        EV_DETAIL << "Master: Received data from slave in another cluster" << endl;
                        EV_DETAIL << "Master: Deleting packet, continuing to wait for next << expectedSlaveDataPackets"
                                  << " slave data packets" << endl;

                        delete mac;

                        macState = WAIT_SLAVE_DATA;
                        // Radio remains in receiving state

                        EV_DETAIL << "Master: Old: WAIT_SLAVE_DATA, New: WAIT_SLAVE_DATA" << endl;

                        break;
                    }
                    // The sending slave is not going to send any more packets
                    else if (isLastPacket)
                        expectedSlaveDataPackets--;

                    // Handle packet
                    if (!containsData)
                    {
                        EV_DETAIL << "Master: Slave data is empty" << endl;
                        delete mac;
                    }
                    else {
                        if (dest == myAddress || dest.isBroadcast() || dest.isMulticast() || dest == CUBEMAC_BROADCAST) {
                            EV_DETAIL << "Master: Sending slave data up" << endl;

                            sendUp(decapsMsg(mac));
                        }
                        else {
                            EV_DETAIL << "Master: Slave data not broadcast or for me, deleting" << endl;
                            delete mac;
                        }
                    }

                    // Check if we need to stay awake for further packets
                    if (expectedSlaveDataPackets > 0) {
                        EV_DETAIL << "Master: Still expecting packets from " << expectedSlaveDataPackets
                                  << " slaves." << endl;

                        // Schedule new timeout
                        scheduleAt(simTime() + timeoutDuration, timeout);

                        // Stay in WAIT_SLAVE_DATA
                        break;
                    }
                    else if (energySavingFeatures) {
                        EV_DETAIL << "Master: Expecting no further slave packets." << endl;

                        macState = SLEEP;
                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                        EV_DETAIL << "Master: Old: WAIT_MASTER_DATA, New: SLEEP" << endl;
                    }
                }

                else if (msg->getKind() == CUBEMAC_WAKEUP) {
                    if (timeout->isScheduled())
                        cancelEvent(timeout);

                    EV_DETAIL << "Master: Waited for slave data until end of slot" << endl;
                    EV_DETAIL << "Master: Was expecting packets from " << expectedSlaveDataPackets
                              << " slave(s)." << endl;

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Master: Old: WAIT_MASTER_DATA, New: SLEEP" << endl;

                    if (wakeUp->isScheduled())
                        cancelEvent(wakeUp);
                    scheduleAt(simTime(), wakeUp);
                }
                else {
                    EV << "Master: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }

                break;

            //---------------//

            case SEND_DATA:
                if (msg->getKind() == CUBEMAC_SEND_DATA) {
                    if (macQueue.size() > 0) {
                        /* Start Errors */
                        if (currSlot != mySlot)
                            throw cRuntimeError("Master: ERROR: Attempting to send data in a slot which is not our own. Repairs needed.\n");
                        /* End Errors */

                        CubeMacFrame *data = macQueue.front()->dup();

                        delete macQueue.front();
                        macQueue.pop_front();

                        data->setContainsData(true);
                        data->setKind(CUBEMAC_MASTER_DATA);
                        data->setClusterId(myClusterId);

                        data->setIsLastPacket(true);
                        canSendNextPacket = false; // There must be a next packet and enough time to send it for this to true

                        // Current packet
                        double bitLength = (double) data->getBitLength();
                        double estTimeToSend = bitLength/bitrate;

                        if ((simTime() + estTimeToSend) >= (currentSlotEndTime + slotPadding)) {
                            throw cRuntimeError("Master: ERROR: Cannot send current packet before end of slot");
                        }

                        // Checking if we can send the next packet on our queue before the slot ends
                        if (macQueue.size() > 0) {
                            // Next packet
                            double nextBitLength = (double) macQueue.front()->getBitLength();
                            double nextEstTimeToSend = nextBitLength/bitrate;

                            simtime_t endNext = estTimeToSend + nextEstTimeToSend;

                            // If next
                            if ((simTime() + endNext) < (currentSlotEndTime + slotPadding)) {
                                EV << "Master: Have calculated that there is time to send next packet on queue " << endl;

                                canSendNextPacket = true;
                                data->setIsLastPacket(false); // Let receiver know we are going to attempt to send another packet
                            }
                        }

                        const MACAddress& dest = data->getDestAddr();

                        EV << "Master: Sending packet '" << data << "' to dest " << dest << endl;

                        simtime_t adm = simTime() - data->getArrivalTimeMac();
                        accessDelayMAC.record(adm);

                        sendDown(data);

                    }
                    // No data to send
                    else {
                        canSendNextPacket = false; // No packets in queue

                        // Create new empty packet
                        CubeMacFrame *data = new CubeMacFrame();

                        data->setContainsData(false);
                        data->setKind(CUBEMAC_MASTER_DATA);
                        data->setClusterId(myClusterId);

                        data->setIsLastPacket(true); // Empty data is always the last packet

                        data->setSrcAddr(myAddress);
                        data->setDestAddr(CUBEMAC_BROADCAST);

                        // ??? Is header just blank?
                        data->setBitLength(headerLength);

                        EV << "Master: No data on queue. Sending empty broadcast packet"<< endl;

                        sendDown(data);
                    }
                }
                else if (msg->getKind() == CUBEMAC_WAKEUP) {
                    throw cRuntimeError("Master: New slot starting while still transmitting data. Check data packet size and/or bitrate\n");
                }
                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            default:
                throw cRuntimeError("Master: Unknown mac state: %d", macState);
                break;

        } // END MASTER FSM
    } // END IF SLAVE ELSE ...
} // END HANDLE SELF MESSAGE

//
// Handle(Upper/Lower)Packet
//

/**
 * Check whether the queue is not full: if yes, print a warning and drop the packet.
 * Sending of messages is automatic.
 */
// Should be same for Master / Slave
// Just adds to macQueue which is of type MacQueue
void CubeMacLayer::handleUpperPacket(cPacket *msg)
{
//    std::string packetClass = msg->getClassName();
//    bool isDymo = (packetClass.std::string::find("dymo") != std::string::npos);

    // TODO: Modify casting to be more flexible
    CubeMacFrame *mac = static_cast<CubeMacFrame *>(encapsMsg(static_cast<cPacket *>(msg)));

//    if (isDymo)
//        mac->setBitLength(1); // bit length of cPacket *macFrame used by IdealTransmitter to calculate duration

    // message has to be queued if another message is waiting to be send
    // or if we are already trying to send another message
    if (macQueue.size() <= queueLength) {
        macQueue.push_back(mac);

        mac->setArrivalTimeMac(simTime());

        EV_DETAIL << "Packet '" << mac << "' placed on queue\n  queue size: " << macQueue.size() << " macState: " << macState << endl;
    }
    else {
        // queue is full, message has to be deleted
        EV_DETAIL << "WARNING: Queue is full, forced to delete packet.\n";
        delete mac;
    }

    packetsOnQueue = macQueue.size();
}

// Just passes packets into the FSM
void CubeMacLayer::handleLowerPacket(cPacket *msg)
{
    if (msg->hasBitError()) {
        EV << "Received " << msg << " which contains bit errors or collision, dropping it\n";
        delete msg;
        return;
    }
    // simply pass the massage as self message, to be processed by the FSM.
    handleSelfMessage(msg);
}

cObject *CubeMacLayer::setUpControlInfo(cMessage *const pMsg, const MACAddress& pSrcAddr)
{
    SimpleLinkLayerControlInfo *const cCtrlInfo = new SimpleLinkLayerControlInfo();
    cCtrlInfo->setSrc(pSrcAddr);
    cCtrlInfo->setInterfaceId(interfaceEntry->getInterfaceId());
    pMsg->setControlInfo(cCtrlInfo);
    return cCtrlInfo;
}

//
// Signals
//
void CubeMacLayer::receiveSignal(cComponent *source, simsignal_t signalID, long value DETAILS_ARG)
{
    // Detecting radio state transitions from transmitting to idle
    if (signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;

        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {

            // Finished sending packet but have also calculated that there is enough time to send the next packet
            if (canSendNextPacket) {
                macState = SEND_DATA;
                radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

                scheduleAt(simTime(), sendData);

                EV_DETAIL << "Signal: Radio set to transmitter mode" << endl;
            }

            // Just finished sending a packet marked as last packet
            else {
                EV_DETAIL << "Signal: Transmission over. Last packet sent. Going to sleep." << endl;

                macState = SLEEP;
                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                EV_DETAIL << "Old state: SEND_DATA, New state: SLEEP" << endl;
            }
        }

        transmissionState = newRadioTransmissionState;
    }

    // Detecting switch to transmitter mode
    else if (signalID == IRadio::radioModeChangedSignal) {
        IRadio::RadioMode radioMode = (IRadio::RadioMode)value;

        if (macState == SEND_DATA && radioMode == IRadio::RADIO_MODE_TRANSMITTER) {

            EV_DETAIL << "Radio in transmitter mode" << endl;
            EV_DETAIL << "Sending self-message to trigger sending of data packet" << endl;

            scheduleAt(simTime(), sendData);
        }
    }
}

// What do they mean here by signal?
void CubeMacLayer::attachSignal(CubeMacFrame *macPkt)
{
    //calculate signal duration
    simtime_t duration = macPkt->getBitLength() / bitrate;
    //create and initialize control info with new signal
    macPkt->setDuration(duration);
}

//
// Decap / Encap
//

cPacket *CubeMacLayer::decapsMsg(CubeMacFrame *msg)
{
//    const MACAddress& dest = msg->getDestAddr();

    cPacket *m = msg->decapsulate();
    setUpControlInfo(m, msg->getSrcAddr());
    // delete the macPkt // ?
    delete msg;
    EV_DETAIL << "Message decapsulated" << endl;

//    if (dest.isMulticast()) {
//
//    }
    return m;
}

/**
 * Encapsulates the received network-layer packet into a MacPkt and set all needed
 * header fields.
 */
CubeMacFrame *CubeMacLayer::encapsMsg(cPacket *netwPkt)
{
    CubeMacFrame *pkt = new CubeMacFrame(netwPkt->getName(), netwPkt->getKind());
    pkt->setBitLength(headerLength);

    // copy dest address from the Control Info attached to the network
    // message by the network layer
    IMACProtocolControlInfo *cInfo = check_and_cast<IMACProtocolControlInfo *>(netwPkt->removeControlInfo());
    EV_DETAIL << "CInfo removed, mac address = " << cInfo->getDestinationAddress() << endl;
    pkt->setDestAddr(cInfo->getDestinationAddress());

    //delete the control info
    delete cInfo;

    //set the src address to own mac address (nic module getId())
    pkt->setSrcAddr(myAddress);

    //encapsulate the network packet
    pkt->encapsulate(netwPkt);
    EV_DETAIL << "Packet encapsulated\n";

    return pkt;
}

//
// Queue
//
void CubeMacLayer::flushQueue()
{
    macQueue.clear();
}

void CubeMacLayer::clearQueue()
{
    macQueue.clear();
}

} // namespace inet
