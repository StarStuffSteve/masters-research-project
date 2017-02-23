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

// --- Might use again in future
//#define myId    (getParentModule()->getParentModule()->getIndex())

// --- MAC Address used in control/data messages to signal nodes that slot owner has no data to send
const MACAddress CubeMacLayer::CUBEMAC_NO_DATA = MACAddress(-2);

void CubeMacLayer::initialize(int stage)
{
    MACProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        //
        // --- Added
        //
        masterId = par("masterId"); // --- Used to assigned fixed slots to masters
        slaveId = par("slaveId"); // --- May need to offset transmission within uplink slot

        startTime = par("startTime").doubleValue(); // --- Used to synchronize nodes

        isSlave = par("isSlave");

        if (isSlave)
            myId = slaveId;
        else
            myId = masterId;

        myClusterId = par("clusterId");

        slavesInCluster = par("slavesInCluster");

        expectedSlaveDataPackets = slavesInCluster; // --- *Must* be reset during sleep

        // ---

        queueLength = par("queueLength");

        slotDuration = par("slotDuration");

        bitrate = par("bitrate");

        headerLength = par("headerLength");
        EV << "headerLength is: " << headerLength << endl;

        numSlots = par("numSlots");

        EV_DETAIL << "My Mac address is" << address << " and my Id is " << myId << endl;

        macState = INIT;

        // TODO: Add vectors
        slotChange = new cOutVector("slotChange");

        // how long does it take to send/receive a control packet
        // --- Where does 16 come from?
        // --- Control packet length / bitrate
        controlDuration = ((double)headerLength + (double)numSlots + 16) / (double)bitrate;
        EV << "Control packets take : " << controlDuration << " seconds to transmit\n";

        initializeMACAddress();

        // --- Comes from MACProtocolBase
        registerInterface();

        // --- NB: Signal subscriptions for receiveSignal function
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);

        WATCH(macState);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        // --- Why commented?
        //int channel;
        //channel = hasPar("defaultChannel") ? par("defaultChannel") : 0;

        EV_DETAIL << " startTime = " << startTime
                  << " isSlave = " << isSlave
                  << " slavesInCluster = " << slavesInCluster
                  << " myId = " << myId
                  << " queueLength = " << queueLength
                  << " slotDuration = " << slotDuration
                  << " controlDuration = " << controlDuration
                  << " numSlots = " << numSlots
                  << " bitrate = " << bitrate << endl;

        // --- Creating initial self messages
        startCubemac = new cMessage("startCubemac");
        startCubemac->setKind(CUBEMAC_START_CUBEMAC);

        timeout = new cMessage("timeout");
        timeout->setKind(CUBEMAC_TIMEOUT);

        wakeUp = new cMessage("wakeUp");
        wakeUp->setKind(CUBEMAC_WAKEUP);

        sendData = new cMessage("sendData");
        sendData->setKind(CUBEMAC_SEND_DATA);

        sendControl = new cMessage("sendControl");
        sendControl->setKind(CUBEMAC_SEND_CONTROL);

        // --- TODO: Just make this 0.0? Does it really matter?
        scheduleAt(startTime, startCubemac);
    }
}

CubeMacLayer::~CubeMacLayer()
{
    // --- cOutVector for vector output file
    delete slotChange;

    cancelAndDelete(startCubemac);
    cancelAndDelete(timeout);
    cancelAndDelete(wakeUp);
    cancelAndDelete(sendData);
    cancelAndDelete(sendControl);

    // --- Clearing the queue
    for (auto & elem : macQueue) {
        delete (elem);
    }
    macQueue.clear();
}

// --- Same as IdealMac
void CubeMacLayer::initializeMACAddress()
{
    const char *addrstr = par("address");

    if (!strcmp(addrstr, "auto")) { // --- If == "auto"
        address = MACAddress::generateAutoAddress();

        // change module parameter from "auto" to concrete address
        par("address").setStringValue(address.str().c_str());
    }
    else {
        address.setAddress(addrstr);
    }
}

// TODO Read more into Interfaces
// --- Same as IdealMac
InterfaceEntry *CubeMacLayer::createInterfaceEntry()
{
    InterfaceEntry *e = new InterfaceEntry(this);
    // --- Add param set/get methods here?
    // --- Who is using this i/f?
    e->setDatarate(bitrate);
    // generate a link-layer address to be used as interface token for IPv6
    e->setMACAddress(address);
    e->setInterfaceToken(address.formInterfaceIdentifier());
    e->setMtu(par("mtu").longValue());
    e->setMulticast(false);
    e->setBroadcast(true);
    return e;
}

void CubeMacLayer::handleSelfMessage(cMessage *msg)
{
    int uplinkSlot = (numSlots - 1);

    if (isSlave)
    {
        switch (macState) {
            case INIT:
                if (msg->getKind() == CUBEMAC_START_CUBEMAC) {
                    mySlot = uplinkSlot;

                    currSlot = -1; // Will be woken up first in slot 0

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    scheduleAt(simTime() + slotDuration, wakeUp);

                    EV_DETAIL << "Slave: First wake up at: " << simTime() + slotDuration << endl;
                    EV_DETAIL << "Slave: Old: INIT, New: SLEEP" << endl;
                }
                else {
                    EV << "Slave: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }
                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            case SLEEP:
                if (msg->getKind() == CUBEMAC_WAKEUP) {
                    currSlot++;
                    currSlot %= numSlots; // 0 .. (numSlots - 1)

                    EV_DETAIL << "Slave: In slot: " << currSlot << " My slot: " << mySlot << endl;

                    // TODO: Modify Master code so it is every Nth slot
                    // if ((currSlot%(numSlots - 1)) == 0) {

                    // ---> SEND_DATA
                    if (currSlot == uplinkSlot) {
                        EV_DETAIL << "Slave: Woken during uplink slot.\n";

                        macState = SEND_DATA;
                        radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

                        EV_DETAIL << "Slave: Old: SLEEP, New: SEND_DATA" << endl;
                    }
                    // --- Slave awaits control in all non-uplink slots
                    // ---> WAIT_CONROL
                    else {
                        EV_DETAIL << "Slave: Woken during non-uplink slot.\n";

                        macState = WAIT_CONTROL;
                        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

                        // --- Wait for duration of timeout for a control message
                        // TODO: Adjust controlDuration?
                        scheduleAt(simTime() + 2.f * controlDuration, timeout);

                        EV_DETAIL << "Slave: Old: SLEEP, New: WAIT_CONTROL" << endl;
                    }
                }
                else {
                    EV << "Slave: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }

                // --- Always schedule the next wakeUp
                scheduleAt(simTime() + slotDuration, wakeUp);

                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            case WAIT_CONTROL:
                /* Start Errors */
                if (msg->getKind() == CUBEMAC_SLAVE_DATA)
                    throw cRuntimeError("Slave: Expected master control received slave data\n");

                if (msg->getKind() == CUBEMAC_DATA)
                    throw cRuntimeError("Slave: Expected master control received master data\n");
                /* End Errors */

                if (msg->getKind() == CUBEMAC_TIMEOUT) {
                    EV_DETAIL << "Slave: Control timeout. Going back to sleep.\n";

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Slave: Old: WAIT_CONTROL, New: SLEEP" << endl;
                }

                else if (msg->getKind() == CUBEMAC_CONTROL) {
                    // TODO: Check out sim manual and other (conditional?) casting functions
                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();

                    EV_DETAIL << "Slave: Received master control packet from src " << mac->getSrcAddr() << " with dest " << dest << endl;

                    if (dest == CUBEMAC_NO_DATA)
                        EV_DETAIL << "Slave: Master has signaled it has no data to send with packet with dest: " << dest << endl;

                    if (dest == address || dest.isBroadcast()) {
                        EV_DETAIL << "Slave: Master data incoming.\n";

                        macState = WAIT_DATA;

                        EV_DETAIL << "Slave: Old: WAIT_CONTROL, New: WAIT_DATA" << endl;

                        if (timeout->isScheduled())
                            cancelEvent(timeout);
                    }
                    else {
                        EV_DETAIL << "Slave: No incoming master data for me. Going back to sleep.\n";

                        macState = SLEEP;
                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                        EV_DETAIL << "Slave: Old: WAIT_CONTROL, New: SLEEP" << endl;

                        if (timeout->isScheduled())
                            cancelEvent(timeout);
                    }
                    delete mac;
                }
                else if ((msg->getKind() == CUBEMAC_WAKEUP)) {
                    EV_DETAIL << "Slave: Stuck in wait control until end of slot";

                    macState = SLEEP;

                    EV_DETAIL << "Slave: Old: WAIT_DATA, New: SLEEP" << endl;

                    scheduleAt(simTime(), wakeUp); // --- Wake up immediately
                }
                else {
                    EV << "Slave: Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }

                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            case WAIT_DATA:
                /* Start Errors */
                if (msg->getKind() == CUBEMAC_CONTROL)
                    throw cRuntimeError("Slave: Expecting master data, received master control.\n");

                if (msg->getKind() == CUBEMAC_SLAVE_DATA)
                    throw cRuntimeError("Slave: Expecting master data, received slave data.\n");
                /* End Errors */

                if (msg->getKind() == CUBEMAC_DATA) {
                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();
                    // --- Not checking for cluster ID, can receive from any master during this slot

                    EV_DETAIL << " Slave: Received a data packet.\n";

                    if (dest == address || dest.isBroadcast()) {
                        EV_DETAIL << "Slave: Sending packet up\n";
                        sendUp(decapsMsg(mac));
                    }
                    else {
                        // --- Shouldn't happen thanks to control packet from master
                        EV_DETAIL << "Slave: Packet not for me, deleting\n";
                        delete mac;
                    }

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Slave: Old: WAIT_DATA, New: SLEEP" << endl;

                    if (timeout->isScheduled())
                        cancelEvent(timeout);
                }
                else if (msg->getKind() == CUBEMAC_WAKEUP) {
                    // TODO: Accept multiple packets within slot
                    EV_DETAIL << "Slave: Stuck waiting for data until end of slot" << endl;

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Slave: Old: WAIT_DATA, New: SLEEP" << endl;

                    scheduleAt(simTime(), wakeUp);
                }
                else {
                    EV << "Slave: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }

                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            // --- Only leads to sleep when transmission finishes
            case SEND_DATA:

                // --- Scheduled by receiveSignal() on radio mode change
                if (msg->getKind() == CUBEMAC_SEND_DATA) {
                    if (macQueue.size() > 0) {
                        // --- Copy next packet on queue
                        CubeMacFrame *data = macQueue.front()->dup();
                        data->setKind(CUBEMAC_SLAVE_DATA);
                        data->setClusterId(myClusterId);

                        EV << "Slave: Sending data packet" << endl;
                        sendDown(data);

                        delete macQueue.front(); // --- Frees pointer and deletes
                        macQueue.pop_front(); //  --- Moves to new pointer?
                    }
                    // --- No data to send
                    else {
                        // --- Create new empty packet
                        CubeMacFrame *data = new CubeMacFrame();
                        data->setKind(CUBEMAC_SLAVE_DATA);
                        data->setClusterId(myClusterId);

                        data->setSrcAddr(address);
                        data->setDestAddr(CUBEMAC_NO_DATA); // --- Signal master that there is no data from this slave

                        data->setBitLength(headerLength + numSlots); // ??? Remove numSlots?

                        EV << "Slave: No data on queue. Sending empty packet"<< endl;
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

    // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //
    // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //
    // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

    else
    {
        switch (macState) {
            case INIT:
                if (msg->getKind() == CUBEMAC_START_CUBEMAC) {
                    mySlot = masterId; // --- Fixed time slot

                    currSlot = -1; // --- Will be woken up first in slot 0

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    scheduleAt(simTime() + slotDuration, wakeUp);

                    EV_DETAIL << "Master: First wake up at: " << simTime() + slotDuration << endl;
                    EV_DETAIL << "Master: Old: INIT, New: SLEEP" << endl;
                }
                else {
                    EV << "Master: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }
                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            case SLEEP:

                // --- Used in wait slave data to count received slave packets
                expectedSlaveDataPackets = slavesInCluster;

                if (msg->getKind() == CUBEMAC_WAKEUP) {
                    currSlot++;
                    currSlot %= numSlots;

                    EV_DETAIL << "Master: In slot: " << currSlot << " My slot: " << mySlot << endl;

                    // ---> WAIT_SLAVE_DATA
                    if (currSlot == uplinkSlot){
                        EV_DETAIL << "Master: Woken during uplink slot." << endl;
                        EV_DETAIL << "Master: expecting " << expectedSlaveDataPackets << " data packets from slaves " << endl;

                        macState = WAIT_SLAVE_DATA;
                        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

                        EV_DETAIL << "Master: Old state: SLEEP, New state: WAIT_SLAVE_DATA" << endl;

                        scheduleAt(simTime() + 2.f * controlDuration, timeout);
                    }
                    // ---> SEND_CONTROL
                    else if (mySlot == currSlot) {
                        EV_DETAIL << "Master: Woken during my slot.\n";

                        macState = SEND_CONTROL;
                        radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

                        EV_DETAIL << "Master: Old: SLEEP, New: SEND_CONTROL" << endl;
                    }
                    // ---> WAIT_CONTROL
                    else {
                        EV_DETAIL << "Master: Woken up in slot of another master. Getting ready to receive master control.\n";

                        macState = WAIT_CONTROL;
                        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

                        EV_DETAIL << "Master: Old state: SLEEP, New state: WAIT_CONTROL" << endl;

                        scheduleAt(simTime() + 2.f * controlDuration, timeout);
                    }
                }
                else {
                    EV << "Master: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }

                scheduleAt(simTime() + slotDuration, wakeUp);

                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            case WAIT_CONTROL:
                /* Start Errors */
                if (msg->getKind() == CUBEMAC_SLAVE_DATA)
                    throw cRuntimeError("Master: Expected master control received slave data\n");

                if (msg->getKind() == CUBEMAC_DATA)
                    throw cRuntimeError("Master: Expected master control received master data\n");
                /* End Errors */

                if (msg->getKind() == CUBEMAC_TIMEOUT) {
                    EV_DETAIL << "Master: Control timeout. Go back to sleep" << endl;

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Master: Old state: WAIT_CONTROL, New state: SLEEP" << endl;
                }

                if (msg->getKind() == CUBEMAC_CONTROL) {
                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();

                    EV_DETAIL << "Master: Received master control packet from src " << mac->getSrcAddr() << " with dest " << dest << endl;

                    // --- Cancel timeout
                    if (timeout->isScheduled())
                        cancelEvent(timeout);

                    if (dest == CUBEMAC_NO_DATA)
                        EV_DETAIL << "Master: Master has signaled it has no data to send with packet with dest: " << dest << endl;

                    if (dest == address || dest.isBroadcast()/* || dest == CUBEMAC_BROADCAST*/) {
                        EV_DETAIL << "Master: Data incoming from master" << endl;

                        macState = WAIT_DATA;

                        EV_DETAIL << "Master: Old: WAIT_CONTROL, New: WAIT_DATA" << endl;
                    }
                    else {
                        EV_DETAIL << "Master: No incoming master data for me. Going back to sleep.\n";

                        macState = SLEEP;
                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                        EV_DETAIL << "Master: Old: WAIT_CONTROL, New: SLEEP" << endl;
                    }
                    delete mac;
                }

                else if ((msg->getKind() == CUBEMAC_WAKEUP)) {
                    EV_DETAIL << "Master: WARNING: left in WAIT_CONTROL until end of slot" << endl;

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Master: Old: WAIT_CONTROL, New: SLEEP" << endl;

                    scheduleAt(simTime(), wakeUp); // --- Wake up immediately
                }

                else {
                    EV << "Master: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }

                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            case WAIT_DATA:
                // --- All nodes must obey the same schedule
                /* Start Errors */
                if (msg->getKind() == CUBEMAC_SLAVE_DATA)
                    throw cRuntimeError("Master: Expected master data received slave data\n");

                if (msg->getKind() == CUBEMAC_CONTROL)
                    throw cRuntimeError("Master: Expected master data received master control\n");
                /* End Errors */

                if (msg->getKind() == CUBEMAC_DATA) {
                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();

                    EV_DETAIL << "Master: Received data from another master " << endl;

                    if (dest == address || dest.isBroadcast()) {
                        EV_DETAIL << "Master: Sending data up...\n";
                        sendUp(decapsMsg(mac));
                    }
                    else {
                        EV_DETAIL << "Master: Data not for me, deleting...\n";
                        delete mac;
                    }

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Master: Old: WAIT_DATA, New: SLEEP" << endl;

                    if (timeout->isScheduled())
                        cancelEvent(timeout);
                }

                else if (msg->getKind() == CUBEMAC_WAKEUP) {
                    EV_DETAIL << "Master: Slot " << currSlot
                              << " ended while waiting for data" << endl;

                    macState = SLEEP;

                    EV_DETAIL << "Master: Old: WAIT_DATA, New: SLEEP" << endl;

                    scheduleAt(simTime(), wakeUp);
                }

                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }

                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            case WAIT_SLAVE_DATA:
                /* Start Errors */
                if (msg->getKind() == CUBEMAC_DATA)
                    throw cRuntimeError("Master: Expected slave data received master data\n");

                if (msg->getKind() == CUBEMAC_CONTROL)
                    throw cRuntimeError("Master: Expected slave data received master control\n");
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
                    // --- Cancel any timeout
                    if (timeout->isScheduled())
                        cancelEvent(timeout);

                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();
                    const int clusterId = mac->getClusterId();

                    // --- Ignore data from outside of my cluster
                    if (clusterId != myClusterId) {
                        EV_DETAIL << "Master: Received data from slave in another cluster" << endl;
                        EV_DETAIL << "Master: Deleting packet, continuing to wait for expected slave data" << endl;

                        delete mac;

                        macState = WAIT_SLAVE_DATA;
                        // --- Keep radio in receiving state

                        EV_DETAIL << "Master: Old: WAIT_SLAVE_DATA, New: WAIT_SLAVE_DATA" << endl;

                        break;
                    }
                    // --- Packet came from within my cluster decrement expected packets
                    else
                        expectedSlaveDataPackets--;

                    EV_DETAIL << "Master: Received data from slave in cluster" << endl;

                    // --- Handle packet
                    if (dest == address || dest.isBroadcast()) {
                        EV_DETAIL << "Master: Sending slave data up" << endl;
                        sendUp(decapsMsg(mac));
                    }
                    else {
                        // --- Could be addressed to someone else or slave may have no data to send
                        EV_DETAIL << "Master: No slave data for me, deleting" << endl;
                        delete mac;
                    }

                    if (expectedSlaveDataPackets > 0)
                    {
                        EV_DETAIL << "Master: Expecting a further " << expectedSlaveDataPackets
                                  << " slave control packet(s)." << endl;

                        // --- Schedule new timeout
                        scheduleAt(simTime() + 2.f * controlDuration, timeout);

                        // --- Stay in WAIT_SLAVE_DATA
                        break;
                    }
                    else
                    {
                        EV_DETAIL << "Master: Expecting no further slave control packets." << endl;

                        macState = SLEEP;
                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                        EV_DETAIL << "Master: Old: WAIT_DATA, New: SLEEP" << endl;
                    }
                }

                else if (msg->getKind() == CUBEMAC_WAKEUP) {
                    EV_DETAIL << "Master: Slot " << currSlot
                              << " ended while waiting for data" << endl;

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Master: Old: WAIT_DATA, New: SLEEP" << endl;

                    scheduleAt(simTime(), wakeUp);
                }
                else {
                    EV << "Master: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }

                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            case SEND_CONTROL:

                if (msg->getKind() == CUBEMAC_SEND_CONTROL) {
                    EV << "Master: Sending a control packet.\n";

                    CubeMacFrame *control = new CubeMacFrame();
                    control->setKind(CUBEMAC_CONTROL);

                    if (macQueue.size() > 0)
                        control->setDestAddr((macQueue.front())->getDestAddr());
                    else
                        control->setDestAddr(CUBEMAC_NO_DATA); // --- No data to send

                    control->setSrcAddr(address);
                    control->setClusterId(myClusterId);
                    control->setBitLength(headerLength + numSlots); // ??? Should this be changed?

                    sendDown(control);

                    // --- Schedule the sending of data
                    if (macQueue.size() > 0)
                        scheduleAt(simTime() + controlDuration, sendData);
                }

                // --- Actual data sending
                else if (msg->getKind() == CUBEMAC_SEND_DATA) {
                    /* Start Errors */
                    if (currSlot != mySlot)
                        throw cRuntimeError("Master: ERROR: Attempting to send data in a slot which is not our own. Repairs needed.\n");
                    /* End Errors */

                    CubeMacFrame *data = macQueue.front()->dup();
                    data->setKind(CUBEMAC_DATA);

                    EV << "Sending down data packet\n";
                    sendDown(data);

                    delete macQueue.front();
                    macQueue.pop_front();

                    macState = SEND_DATA;

                    EV_DETAIL << "Master: Old: SEND_CONTROL, New: SEND_DATA" << endl;
                }
                else {
                    EV << "Master: Unknown packet " << msg->getKind() << " in state " << macState << endl;
                }
                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            case SEND_DATA:
                if (msg->getKind() == CUBEMAC_WAKEUP) {
                    throw cRuntimeError("Master: WARNING: New slot starting while still transmitting data. Check data packet size and/or bitrate\n");
                }
                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- //

            default:
                throw cRuntimeError("Master: Unknown mac state: %d", macState);
                break;

        } // END MASTER FSM
    } // END IF SLAVE ELSE ...
} // END HANDLE SELF MESSAGE

//
// --- Handle(Upper/Lower)Packet
//

/**
 * Check whether the queue is not full: if yes, print a warning and drop the packet.
 * Sending of messages is automatic.
 */
// --- Should be same for Master / Slave
// --- Just adds to macQueue which is of type MacQueue
void CubeMacLayer::handleUpperPacket(cPacket *msg)
{
    CubeMacFrame *mac = static_cast<CubeMacFrame *>(encapsMsg(static_cast<cPacket *>(msg)));

    // message has to be queued if another message is waiting to be send
    // or if we are already trying to send another message
    if (macQueue.size() <= queueLength) {

        macQueue.push_back(mac);
        EV_DETAIL << "Packet put in queue\n  queue size: " << macQueue.size() << " macState: " << macState
                  << "; mySlot is " << mySlot << "; current slot is " << currSlot << endl;
        ;
    }
    else {
        // queue is full, message has to be deleted
        EV_DETAIL << "WARNING: Queue is full, forced to delete packet.\n";
        delete mac;
    }
}

/**
 * Handle LMAC control packets and data packets. Recognize collisions, change own slot if necessary and remember who is using which slot.
 */
// --- Just passes packets into the FSM
void CubeMacLayer::handleLowerPacket(cPacket *msg)
{
    if (msg->hasBitError()) {
        EV << "Received " << msg << " contains bit errors or collision, dropping it\n";
        delete msg;
        return;
    }
    // simply pass the massage as self message, to be processed by the FSM.
    handleSelfMessage(msg);
}


/**
 * Attaches a "control info" (MacToNetw) structure (object) to the message pMsg.
 */
// --- Not sure what this is doing
cObject *CubeMacLayer::setUpControlInfo(cMessage *const pMsg, const MACAddress& pSrcAddr)
{
    SimpleLinkLayerControlInfo *const cCtrlInfo = new SimpleLinkLayerControlInfo();
    cCtrlInfo->setSrc(pSrcAddr);
    cCtrlInfo->setInterfaceId(interfaceEntry->getInterfaceId());
    pMsg->setControlInfo(cCtrlInfo);
    return cCtrlInfo;
}

//
// --- Signals
//

/**
 * Handle transmission over messages: send the data packet or don;t do anything.
 */
// --- Signals that transmission and/or radio states have changed
void CubeMacLayer::receiveSignal(cComponent *source, simsignal_t signalID, long value DETAILS_ARG)
{
    // --- Looking for radio state transitions from transmitting to idle
    // TODO: Will have to modify for the sending of multiple within slot
    if (signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;

        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
            if (sendData->isScheduled()) {
                EV_DETAIL << "Signal: Transmission of control packet over. data transfer will start soon." << endl;
            }

            else {
                EV_DETAIL << "Signal: Transmission over. nothing else is scheduled, get back to sleep." << endl;

                EV_DETAIL << "Old state: ?, New state: SLEEP" << endl;

                macState = SLEEP;
                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                if (timeout->isScheduled())
                    cancelEvent(timeout);
            }
        }

        transmissionState = newRadioTransmissionState; // --- Should be RADIO_MODE_SLEEP?
    }
    else if (signalID == IRadio::radioModeChangedSignal) {
        IRadio::RadioMode radioMode = (IRadio::RadioMode)value;

        if (isSlave && macState == SEND_DATA && radioMode == IRadio::RADIO_MODE_TRANSMITTER) {

            EV_DETAIL << "Slave: Radio set to transmitter mode" << endl;
            EV_DETAIL << "Slave: Send self-message to trigger sending of data packet" << endl;

//            scheduleAt(simTime() + (controlDuration * (slaveId + 1)), sendControl);
            scheduleAt(simTime(), sendData);
        }
        else if (macState == SEND_CONTROL && radioMode == IRadio::RADIO_MODE_TRANSMITTER) {

            EV_DETAIL << "Master: Radio set to transmitter mode" << endl;
            EV_DETAIL << "Master: Send self-message to trigger sending of control packet" << endl;

            scheduleAt(simTime() +  controlDuration, sendControl);
        }
    }
}

// --- What do they mean here by signal?
void CubeMacLayer::attachSignal(CubeMacFrame *macPkt)
{
    //calculate signal duration
    simtime_t duration = macPkt->getBitLength() / bitrate;
    //create and initialize control info with new signal
    macPkt->setDuration(duration);
}

//
// --- Decap / Encap
//

cPacket *CubeMacLayer::decapsMsg(CubeMacFrame *msg)
{
    cPacket *m = msg->decapsulate();
    setUpControlInfo(m, msg->getSrcAddr());
    // delete the macPkt // --- ?
    delete msg;
    EV_DETAIL << "Message decapsulated" << endl;
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
    EV_DETAIL << "CInfo removed, mac addr=" << cInfo->getDestinationAddress() << endl;
    pkt->setDestAddr(cInfo->getDestinationAddress());

    //delete the control info
    delete cInfo;

    //set the src address to own mac address (nic module getId())
    pkt->setSrcAddr(address);

    //encapsulate the network packet
    pkt->encapsulate(netwPkt);
    EV_DETAIL << "pkt encapsulated\n";

    return pkt;
}

//
// --- Queue
//

void CubeMacLayer::flushQueue()
{
    // TODO: ?
    macQueue.clear();
}

void CubeMacLayer::clearQueue()
{
    macQueue.clear();
}

} // namespace inet

