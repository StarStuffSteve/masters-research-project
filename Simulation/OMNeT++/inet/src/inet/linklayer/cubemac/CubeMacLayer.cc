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

// !!! Expecting all hosts to be in the same array?
#define myId    (getParentModule()->getParentModule()->getIndex())

// --- MAC Address used in control messages to signal nodes that slot owner has no data to send
const MACAddress CubeMacLayer::CUBEMAC_NO_RECEIVER = MACAddress(-2);
// --- MAC Address used to signal that a slot is free used in occSlotsAway and occSlotsDirect
const MACAddress CubeMacLayer::CUBEMAC_FREE_SLOT = MACAddress::BROADCAST_ADDRESS;

void CubeMacLayer::initialize(int stage)
{
    MACProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        //
        // --- Added
        //
        isSlave = par("isSlave");
        // ---

        queueLength = par("queueLength");

        slotDuration = par("slotDuration");

        queueLength = par("queueLength");

        slotDuration = par("slotDuration");

        bitrate = par("bitrate");

        headerLength = par("headerLength");
        EV << "headerLength is: " << headerLength << endl;

        numSlots = par("numSlots");
        if (!isSlave)
        {
            numSlots--;
            EV << "Master: effective numSlots " << numSlots << endl;
        }

        // the first N slots are reserved for mobile nodes to be able to function normally
        // --- TODO Remove this
        reservedMobileSlots = par("reservedMobileSlots");

        EV_DETAIL << "My Mac address is" << address << " and my Id is " << myId << endl;
        macState = INIT;

        // --- TODO Add vectors
        slotChange = new cOutVector("slotChange");

        // how long does it take to send/receive a control packet
        // --- Why 16?
        controlDuration = ((double)headerLength + (double)numSlots + 16) / (double)bitrate;
        EV << "Control packets take : " << controlDuration << " seconds to transmit\n";

        initializeMACAddress();

        // --- Comes from MACProtocolBase
        registerInterface();

        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);

        // --- Place watches here
        WATCH(macState);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        // --- Why commented?
        //int channel;
        //channel = hasPar("defaultChannel") ? par("defaultChannel") : 0;

        EV_DETAIL << " isSlave = " << isSlave
                  << " queueLength = " << queueLength
                  << " slotDuration = " << slotDuration
                  << " controlDuration = " << controlDuration
                  << " numSlots = " << numSlots
                  << " bitrate = " << bitrate << endl;

        // --- Creating initial self messages
        timeout = new cMessage("timeout");
        timeout->setKind(CUBEMAC_TIMEOUT);

        sendData = new cMessage("sendData");
        sendData->setKind(CUBEMAC_SEND_DATA);

        wakeup = new cMessage("wakeup");
        wakeup->setKind(CUBEMAC_WAKEUP);

        initChecker = new cMessage("setup phase");
        initChecker->setKind(CUBEMAC_SETUP_PHASE_END);

        checkChannel = new cMessage("checkchannel");
        checkChannel->setKind(CUBEMAC_CHECK_CHANNEL);

        start_cubemac = new cMessage("start_cubemac");
        start_cubemac->setKind(CUBEMAC_START_CUBEMAC);

        send_control = new cMessage("send_control");
        send_control->setKind(CUBEMAC_SEND_CONTROL);

        scheduleAt(0.0, start_cubemac);
    }
}

CubeMacLayer::~CubeMacLayer()
{
    // --- cOutVector for vector output file
    delete slotChange;

    cancelAndDelete(timeout);
    cancelAndDelete(wakeup);
    cancelAndDelete(checkChannel);
    cancelAndDelete(sendData);
    cancelAndDelete(initChecker);
    cancelAndDelete(start_cubemac);
    cancelAndDelete(send_control);

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
        // assign automatic address
        // --- See 'MACAddress'
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

    // data rate
    e->setDatarate(bitrate);

    // generate a link-layer address to be used as interface token for IPv6
    e->setMACAddress(address);

    e->setInterfaceToken(address.formInterfaceIdentifier());

    // capabilities
    e->setMtu(par("mtu").longValue());

    e->setMulticast(false);

    e->setBroadcast(true);

    return e;
}

void CubeMacLayer::handleSelfMessage(cMessage *msg)
{
    if (isSlave)
    {
        switch (macState) {
            case INIT:
                if (msg->getKind() == CUBEMAC_START_CUBEMAC) {
                    // the first 5 full slots we will be waking up every controlDuration to setup the network first
                    // normal packets will be queued, but will be send only after the setup phase
                    // --- Slaves sleep for entire setup phase as they don't occupy 'slots'

                    mySlot = -1;

                    // !!! Slaves will never update these
                    // !!! Masters should never read these in messages from slaves
                    for (int i = 0; i < numSlots; i++) {
                        occSlotsDirect[i] = CUBEMAC_FREE_SLOT; // Bogus data
                        occSlotsAway[i] = CUBEMAC_FREE_SLOT;
                    }

                    macState = SLEEP;
                    scheduleAt(slotDuration * 5 * numSlots, wakeup);

                    EV << "Slave: Sleeping for statup time = " << slotDuration * 5 * numSlots << endl
                    EV_DETAIL << "Slave: Old state: INIT, New state: SLEEP" << endl;
                }
                else {
                    EV << "Slave: Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            case SLEEP:
                if (msg->getKind() == CUBEMAC_WAKEUP) {
                    currSlot++;
                    currSlot %= numSlots;

                    EV_DETAIL << "Slave: New slot starting - No. " << currSlot << endl;

                    // TODO: Modify Master code so it is every Nth slot
                    // if ((currSlot%(numSlots - 1)) == 0) {

                    // --- Get ready to send data in the last slot
                    if (currSlot == (numSlots - 1)) {
                        EV_DETAIL << "Slave: Woken during slave uplink slot.\n";

                        // --- CDMA - No need to sense the channel we're just got right ahead and send
                        // !!! Must make sure the masters don't detect collisions during these slots
                        macState = SEND_CONTROL;

                        radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

                        EV_DETAIL << "Slave: Old state: CCA, New state: SEND_CONTROL" << endl;
                    }
                    // Slave awaits control in all non-uplink slots
                    else {
                        EV_DETAIL << "Slave: Woken during non-uplink slot.\n";

                        macState = WAIT_CONTROL;
                        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

                        // --- Wait for duration of timeout for a control message
                        scheduleAt(simTime() + 2.f * controlDuration, timeout);

                        EV_DETAIL << "Slave: Old state: SLEEP, New state: WAIT_CONTROL" << endl;
                    }

                    // --- Schedule next wakeup
                    scheduleAt(simTime() + slotDuration, wakeup);
                }
                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            case CCA:
                // TODO Raise runtime
                EV << "Slave: ERROR: Slave stuck in CCA state, this should be impossible" << endl;
                break;

            // --- Wating to receive control messages
            case WAIT_CONTROL:
                if (msg->getKind() == CUBEMAC_TIMEOUT) {

                    EV_DETAIL << "Slave: Control timeout. Going back to sleep.\n";

                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Old state: WAIT_CONTROL, New state: SLEEP" << endl;
                }
                else if (msg->getKind() == CUBEMAC_CONTROL) {
                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();

                    EV_DETAIL << "Slave: received control packet from src " << mac->getSrcAddr() << " with dest " << dest << ".\n";

                    // !!! Occupied slot updating and collision code removed

                    if (dest == address || dest.isBroadcast()) {
                        EV_DETAIL << "Slave: Data incoming.\n";

                        macState = WAIT_DATA;

                        EV_DETAIL << "Slave: Old state: WAIT_CONTROL, New state: WAIT_DATA" << endl;

                        if (timeout->isScheduled())
                            cancelEvent(timeout);
                    }
                    else {
                        EV_DETAIL << "Slave: Incoming data not for me. Going back to sleep.\n";

                        macState = SLEEP;
                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                        EV_DETAIL << "Slave: Old state: WAIT_CONTROL, New state: SLEEP" << endl;

                        if (timeout->isScheduled())
                            cancelEvent(timeout);
                    }
                    // Copy of packet no longer needed
                    delete mac;
                }
                else if ((msg->getKind() == CUBEMAC_WAKEUP)) {
                    // !!! Yeah ... but what is the effect? A missed slot?
                    EV_DETAIL << "Slave: 'wakeup' during WAIT_CONTROL. Very unlikely transition";

                    macState = SLEEP;

                    EV_DETAIL << "Old state: WAIT_DATA, New state: SLEEP" << endl;

                    scheduleAt(simTime(), wakeup);
                }
                else {
                    EV << "Slave: Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }

                break;

            case SEND_CONTROL:

                // --- Will get this self message thanks to receiveSignal() and the radio state change
                if (msg->getKind() == CUBEMAC_SEND_CONTROL) {
                    // send first a control message, so that non-receiving nodes can switch off.
                    EV << "Slave: Sending a control packet.\n";

                    CubeMacFrame *control = new CubeMacFrame();
                    control->setKind(CUBEMAC_CONTROL);

                    // --- If slave has data to send
                    if (macQueue.size() > 0)
                        control->setDestAddr((macQueue.front())->getDestAddr());
                    else
                        control->setDestAddr(CUBEMAC_NO_RECEIVER);

                    control->setSrcAddr(address);
                    control->setMySlot(mySlot); // Will always be -1
                    control->setBitLength(headerLength + numSlots);
                    //
                    // !!! Must make sure Masters do no corrupt their data with this
                    //
                    control->setOccupiedSlotsArraySize(numSlots);
                    for (int i = 0; i < numSlots; i++)
                        control->setOccupiedSlots(i, CUBEMAC_FREE_SLOT); // Bogus Data

                    // --- Send control message
                    sendDown(control);

                    // --- If slave has data to send
                    if (macQueue.size() > 0)
                        scheduleAt(simTime() + controlDuration, sendData);
                }

                //
                // !!! Only sending one data packet ...
                //
                // TODO: Added code to allow sending of 2+ packets
                // TODO: Consider "Safe" way of handling/detecting transmission duration > slot duration
                else if (msg->getKind() == CUBEMAC_SEND_DATA) {
                    // We should be in slave uplink slot and our control packet should be already sent.
                    // Receiving neighbors should wait for the data now.
                    if (currSlot != (numSlots - 1)) {
                        EV_DETAIL << "Slave: ERROR: Send data message received while not in slave uplink slot. Repairs needed" << endl;

                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                        if (timeout->isScheduled())
                            cancelEvent(timeout);

                        return; // --- Why not just break?
                    }

                    CubeMacFrame *data = macQueue.front()->dup();

                    data->setKind(CUBEMAC_DATA);
                    data->setMySlot(mySlot); // --- Will be -1
                    //
                    // !!! Must make sure Masters do no corrupt their data with this
                    //
                    data->setOccupiedSlotsArraySize(numSlots);
                    for (int i = 0; i < numSlots; i++)
                        data->setOccupiedSlots(i, occSlotsDirect[i]); // Bogus Data

                    EV << "Slave: Sending data packet down" << endl;
                    sendDown(data);

                    delete macQueue.front(); // Frees pointer and deletes
                    macQueue.pop_front(); // Moves to new pointer?

                    // --- Can only go to SLEEP after going to SEND_DATA
                    macState = SEND_DATA;

                    EV_DETAIL << "Slave: Old state: SEND_CONTROL, New state: SEND_DATA" << endl;
                }
                else {
                    EV << "Slave: Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            // --- Only leads to sleep when finished transmitting, could extend.
            case SEND_DATA:
                // TODO: Perform magic here
                if (msg->getKind() == CUBEMAC_WAKEUP) {
                    throw cRuntimeError("Slave: New slot starting while still transmitting data. Check data packet size and/or bitrate\n\n");
                }
                else {
                    EV << "Slave: Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            case WAIT_DATA:
                if (msg->getKind() == CUBEMAC_DATA) {
                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();

                    EV_DETAIL << " Slave: Received a data packet.\n";
                    if (dest == address || dest.isBroadcast()) {
                        EV_DETAIL << "Slave: Sending pkt to upper\n";
                        sendUp(decapsMsg(mac));
                    }
                    else {
                        EV_DETAIL << "Slave: Packet not for me, deleting\n";
                        delete mac;
                    }

                    // in any case, go back to sleep
                    macState = SLEEP;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                    EV_DETAIL << "Slave: Old state: WAIT_DATA, New state: SLEEP" << endl;

                    if (timeout->isScheduled())
                        cancelEvent(timeout);
                }
                else if (msg->getKind() == CUBEMAC_WAKEUP) {
                    macState = SLEEP;

                    EV_DETAIL << "Slave: Unlikely transition. Old state: WAIT_DATA, New state: SLEEP" << endl;

                    scheduleAt(simTime(), wakeup);
                }
                else {
                    EV << "Slave: Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            default:
                throw cRuntimeError("Slave: Unknown MAC state: %d", macState);
                break;
        } // END SLAVE FSM
    }


    //
    //
    //


    else
    {
        switch (macState) {
            // --- Unchanged
            case INIT:
                if (msg->getKind() == CUBEMAC_START_CUBEMAC) {
                    // the first 5 full frames we will be waking up every controlDuration to setup the network first
                    // normal packets will be queued, but will be send only after the setup phase
                    scheduleAt(slotDuration * 5 * numSlots, initChecker);
                    EV << "Startup time =" << slotDuration * 5 * numSlots << endl;

                    EV_DETAIL << "Scheduling the first wake-up at : " << slotDuration << endl;

                    scheduleAt(slotDuration, wakeup);

                    for (int i = 0; i < numSlots; i++) {
                        occSlotsDirect[i] = CUBEMAC_FREE_SLOT;
                        occSlotsAway[i] = CUBEMAC_FREE_SLOT;
                    }

                    if (myId >= reservedMobileSlots)
                        mySlot = ((int)FindModule<>::findHost(this)->getId()) % (numSlots - reservedMobileSlots);
                    else
                        mySlot = myId;
                    //occSlotsDirect[mySlot] = address;
                    //occSlotsAway[mySlot] = address;
                    currSlot = 0;

                    EV_DETAIL << "ID: " << FindModule<>::findHost(this)->getId() << ". Picked random slot: " << mySlot << endl;

                    macState = SLEEP;
                    EV_DETAIL << "Old state: INIT, New state: SLEEP" << endl;
                    SETUP_PHASE = true;
                }
                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            case SLEEP:
                if (msg->getKind() == CUBEMAC_WAKEUP) {
                    currSlot++;
                    currSlot %= numSlots;
                    EV_DETAIL << "New slot starting - No. " << currSlot << ", my slot is " << mySlot << endl;

                    if (mySlot == currSlot) {
                        EV_DETAIL << "Waking up in my slot. Switch to RECV first to check the channel.\n";
                        macState = CCA;
                        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
                        EV_DETAIL << "Old state: SLEEP, New state: CCA" << endl;

                        double small_delay = controlDuration * dblrand();
                        scheduleAt(simTime() + small_delay, checkChannel);
                        EV_DETAIL << "Checking for channel for " << small_delay << " time.\n";
                    }
                    else {
                        EV_DETAIL << "Waking up in a foreign slot. Ready to receive control packet.\n";
                        macState = WAIT_CONTROL;
                        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
                        EV_DETAIL << "Old state: SLEEP, New state: WAIT_CONTROL" << endl;
                        if (!SETUP_PHASE) //in setup phase do not sleep
                            scheduleAt(simTime() + 2.f * controlDuration, timeout);
                    }
                    if (SETUP_PHASE) {
                        scheduleAt(simTime() + 2.f * controlDuration, wakeup);
                        EV_DETAIL << "setup phase slot duration:" << 2.f * controlDuration << "while control duration is" << controlDuration << endl;
                    }
                    else
                        scheduleAt(simTime() + slotDuration, wakeup);
                }
                else if (msg->getKind() == CUBEMAC_SETUP_PHASE_END) {
                    EV_DETAIL << "Setup phase end. Start normal work at the next slot.\n";
                    if (wakeup->isScheduled())
                        cancelEvent(wakeup);

                    scheduleAt(simTime() + slotDuration, wakeup);

                    SETUP_PHASE = false;
                }
                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            case CCA:
                if (msg->getKind() == CUBEMAC_CHECK_CHANNEL) {
                    // if the channel is clear, get ready for sending the control packet
                    EV << "Channel is free, so let's prepare for sending.\n";

                    macState = SEND_CONTROL;
                    radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
                    EV_DETAIL << "Old state: CCA, New state: SEND_CONTROL" << endl;
                }
                else if (msg->getKind() == CUBEMAC_CONTROL) {
                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();
                    EV_DETAIL << " I have received a control packet from src " << mac->getSrcAddr() << " and dest " << dest << ".\n";
                    bool collision = false;
                    // if we are listening to the channel and receive anything, there is a collision in the slot.
                    if (checkChannel->isScheduled()) {
                        cancelEvent(checkChannel);
                        collision = true;
                    }

                    for (int s = 0; s < numSlots; s++) {
                        occSlotsAway[s] = mac->getOccupiedSlots(s);
                        EV_DETAIL << "Occupied slot " << s << ": " << occSlotsAway[s] << endl;
                        EV_DETAIL << "Occupied direct slot " << s << ": " << occSlotsDirect[s] << endl;
                    }

                    if (mac->getMySlot() > -1) {
                        // check first whether this address didn't have another occupied slot and free it again
                        for (int i = 0; i < numSlots; i++) {
                            if (occSlotsDirect[i] == mac->getSrcAddr())
                                occSlotsDirect[i] = CUBEMAC_FREE_SLOT;
                            if (occSlotsAway[i] == mac->getSrcAddr())
                                occSlotsAway[i] = CUBEMAC_FREE_SLOT;
                        }
                        occSlotsAway[mac->getMySlot()] = mac->getSrcAddr();
                        occSlotsDirect[mac->getMySlot()] = mac->getSrcAddr();
                    }

                    // --- There is a collision if someone has transmitted a control packet during mySlot
                    collision = collision || (mac->getMySlot() == mySlot);

                    // --- (I have a legit slot and ? and myslot does not have my address in it) or there was a collision
                    if (((mySlot > -1) && (mac->getOccupiedSlots(mySlot) > CUBEMAC_FREE_SLOT) && (mac->getOccupiedSlots(mySlot) != address)) || collision) {
                        EV_DETAIL << "My slot is taken by " << mac->getOccupiedSlots(mySlot) << ". I need to change it.\n";
                        findNewSlot();
                        EV_DETAIL << "My new slot is " << mySlot << endl;
                    }
                    // --- I don't have a legit slot
                    if (mySlot < 0) {
                        EV_DETAIL << "I don't have a slot - try to find one.\n";
                        findNewSlot();
                    }
                    // --- Sent a control message to myself or received a broadcast
                    if (dest == address || dest.isBroadcast()) {
                        EV_DETAIL << "I need to stay awake.\n";
                        if (timeout->isScheduled())
                            cancelEvent(timeout);
                        macState = WAIT_DATA;
                        EV_DETAIL << "Old state: CCA, New state: WAIT_DATA" << endl;
                    }
                    else {
                        EV_DETAIL << "Incoming data packet not for me. Going back to sleep.\n";
                        macState = SLEEP;
                        EV_DETAIL << "Old state: CCA, New state: SLEEP" << endl;
                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                        if (timeout->isScheduled())
                            cancelEvent(timeout);
                    }
                    delete mac;
                }
                //probably it never happens
                else if (msg->getKind() == CUBEMAC_DATA) {
                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();
                    // bool collision = false;
                    // if we are listening to the channel and receive anything, there is a collision in the slot.
                    if (checkChannel->isScheduled()) {
                        cancelEvent(checkChannel);
                        //collision = true;
                    }
                    EV_DETAIL << " I have received a data packet.\n";
                    if (dest == address || dest.isBroadcast()) {
                        EV_DETAIL << "sending pkt to upper...\n";
                        sendUp(decapsMsg(mac));
                    }
                    else {
                        EV_DETAIL << "packet not for me, deleting...\n";
                        delete mac;
                    }
                    // in any case, go back to sleep
                    macState = SLEEP;
                    EV_DETAIL << "Old state: CCA, New state: SLEEP" << endl;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                }
                else if (msg->getKind() == CUBEMAC_SETUP_PHASE_END) {
                    EV_DETAIL << "Setup phase end. Start normal work at the next slot.\n";
                    if (wakeup->isScheduled())
                        cancelEvent(wakeup);

                    scheduleAt(simTime() + slotDuration, wakeup);

                    SETUP_PHASE = false;
                }
                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            case WAIT_CONTROL:
                if (msg->getKind() == CUBEMAC_TIMEOUT) {
                    EV_DETAIL << "Control timeout. Go back to sleep.\n";
                    macState = SLEEP;
                    EV_DETAIL << "Old state: WAIT_CONTROL, New state: SLEEP" << endl;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                }
                else if (msg->getKind() == CUBEMAC_CONTROL) {
                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();
                    EV_DETAIL << " I have received a control packet from src " << mac->getSrcAddr() << " and dest " << dest << ".\n";

                    bool collision = false;

                    // check first the slot assignment
                    // copy the current slot assignment

                    for (int s = 0; s < numSlots; s++) {
                        occSlotsAway[s] = mac->getOccupiedSlots(s);
                        EV_DETAIL << "Occupied slot " << s << ": " << occSlotsAway[s] << endl;
                        EV_DETAIL << "Occupied direct slot " << s << ": " << occSlotsDirect[s] << endl;
                    }

                    if (mac->getMySlot() > -1) {
                        // check first whether this address didn't have another occupied slot and free it again
                        for (int i = 0; i < numSlots; i++) {
                            if (occSlotsDirect[i] == mac->getSrcAddr())
                                occSlotsDirect[i] = CUBEMAC_FREE_SLOT;
                            if (occSlotsAway[i] == mac->getSrcAddr())
                                occSlotsAway[i] = CUBEMAC_FREE_SLOT;
                        }
                        occSlotsAway[mac->getMySlot()] = mac->getSrcAddr();
                        occSlotsDirect[mac->getMySlot()] = mac->getSrcAddr();
                    }

                    collision = collision || (mac->getMySlot() == mySlot);
                    if (((mySlot > -1) && (mac->getOccupiedSlots(mySlot) > CUBEMAC_FREE_SLOT) && (mac->getOccupiedSlots(mySlot) != address)) || collision) {
                        EV_DETAIL << "My slot is taken by " << mac->getOccupiedSlots(mySlot) << ". I need to change it.\n";
                        findNewSlot();
                        EV_DETAIL << "My new slot is " << mySlot << endl;
                    }
                    if (mySlot < 0) {
                        EV_DETAIL << "I don;t have a slot - try to find one.\n";
                        findNewSlot();
                    }

                    if (dest == address || dest.isBroadcast()) {
                        EV_DETAIL << "I need to stay awake.\n";
                        macState = WAIT_DATA;
                        EV_DETAIL << "Old state: WAIT_CONTROL, New state: WAIT_DATA" << endl;
                        if (timeout->isScheduled())
                            cancelEvent(timeout);
                    }
                    else {
                        EV_DETAIL << "Incoming data packet not for me. Going back to sleep.\n";
                        macState = SLEEP;
                        EV_DETAIL << "Old state: WAIT_CONTROL, New state: SLEEP" << endl;
                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                        if (timeout->isScheduled())
                            cancelEvent(timeout);
                    }
                    delete mac;
                }
                else if ((msg->getKind() == CUBEMAC_WAKEUP)) {
                    if (SETUP_PHASE == true)
                        EV_DETAIL << "End of setup-phase slot" << endl;
                    else
                        EV_DETAIL << "Very unlikely transition";

                    macState = SLEEP;
                    EV_DETAIL << "Old state: WAIT_DATA, New state: SLEEP" << endl;
                    scheduleAt(simTime(), wakeup);
                }
                else if (msg->getKind() == CUBEMAC_SETUP_PHASE_END) {
                    EV_DETAIL << "Setup phase end. Start normal work at the next slot.\n";
                    if (wakeup->isScheduled())
                        cancelEvent(wakeup);

                    scheduleAt(simTime() + slotDuration, wakeup);

                    SETUP_PHASE = false;
                }
                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }

                break;

            case SEND_CONTROL:

                if (msg->getKind() == CUBEMAC_SEND_CONTROL) {
                    // send first a control message, so that non-receiving nodes can switch off.
                    EV << "Sending a control packet.\n";
                    CubeMacFrame *control = new CubeMacFrame();
                    control->setKind(CUBEMAC_CONTROL);
                    if ((macQueue.size() > 0) && !SETUP_PHASE)
                        control->setDestAddr((macQueue.front())->getDestAddr());
                    else
                        control->setDestAddr(CUBEMAC_NO_RECEIVER);

                    control->setSrcAddr(address);
                    control->setMySlot(mySlot);
                    control->setBitLength(headerLength + numSlots);
                    control->setOccupiedSlotsArraySize(numSlots);
                    for (int i = 0; i < numSlots; i++)
                        control->setOccupiedSlots(i, occSlotsDirect[i]);

                    sendDown(control);
                    if ((macQueue.size() > 0) && (!SETUP_PHASE))
                        scheduleAt(simTime() + controlDuration, sendData);
                }
                else if (msg->getKind() == CUBEMAC_SEND_DATA) {
                    // we should be in our own slot and the control packet should be already sent. receiving neighbors should wait for the data now.
                    if (currSlot != mySlot) {
                        EV_DETAIL << "ERROR: Send data message received, but we are not in our slot!!! Repair.\n";
                        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                        if (timeout->isScheduled())
                            cancelEvent(timeout);
                        return;
                    }
                    CubeMacFrame *data = macQueue.front()->dup();
                    data->setKind(CUBEMAC_DATA);
                    data->setMySlot(mySlot);
                    data->setOccupiedSlotsArraySize(numSlots);
                    for (int i = 0; i < numSlots; i++)
                        data->setOccupiedSlots(i, occSlotsDirect[i]);

                    EV << "Sending down data packet\n";
                    sendDown(data);
                    delete macQueue.front();
                    macQueue.pop_front();
                    macState = SEND_DATA;
                    EV_DETAIL << "Old state: SEND_CONTROL, New state: SEND_DATA" << endl;
                }
                else if (msg->getKind() == CUBEMAC_SETUP_PHASE_END) {
                    EV_DETAIL << "Setup phase end. Start normal work at the next slot.\n";
                    if (wakeup->isScheduled())
                        cancelEvent(wakeup);

                    scheduleAt(simTime() + slotDuration, wakeup);

                    SETUP_PHASE = false;
                }
                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            case SEND_DATA:
                if (msg->getKind() == CUBEMAC_WAKEUP) {
                    throw cRuntimeError("Master: New slot starting while still transmitting data. Check data packet size and/or bitrate\n");
                }
                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            case WAIT_DATA:
                if (msg->getKind() == CUBEMAC_DATA) {
                    CubeMacFrame *const mac = static_cast<CubeMacFrame *>(msg);
                    const MACAddress& dest = mac->getDestAddr();

                    EV_DETAIL << " I have received a data packet.\n";
                    if (dest == address || dest.isBroadcast()) {
                        EV_DETAIL << "sending pkt to upper...\n";
                        sendUp(decapsMsg(mac));
                    }
                    else {
                        EV_DETAIL << "packet not for me, deleting...\n";
                        delete mac;
                    }
                    // in any case, go back to sleep
                    macState = SLEEP;
                    EV_DETAIL << "Old state: WAIT_DATA, New state: SLEEP" << endl;
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                    if (timeout->isScheduled())
                        cancelEvent(timeout);
                }
                else if (msg->getKind() == CUBEMAC_WAKEUP) {
                    macState = SLEEP;
                    EV_DETAIL << "Unlikely transition. Old state: WAIT_DATA, New state: SLEEP" << endl;
                    scheduleAt(simTime(), wakeup);
                }
                else {
                    EV << "Unknown packet" << msg->getKind() << "in state" << macState << endl;
                }
                break;

            default:
                throw cRuntimeError("Unknown mac state: %d", macState);
                break;

        } // END MASTER FSM
    } // END IF SLAVE ELSE ...
} // END HANDLE SELF MESSAGE

/**
 * Try to find a new slot after collision. If not possible, set own slot to -1 (not able to send anything)
 */
// --- So long as the parameter given for numSlots > numMasters then we should always be able to find a free slot
void CubeMacLayer::findNewSlot()
{
    // --- Pick the first slot
    mySlot = 0;

    // --- Find the first free slot
    // --- Why Away and not Direct?
    while ((occSlotsAway[mySlot] != CUBEMAC_FREE_SLOT) && (mySlot < numSlots)) {
        mySlot++;
    }

    // --- Check if found a slot
    if (mySlot == numSlots) {
        EV << "Master: FAILURE: Cannot find a free slot. Cannot send data.\n" << endl;
        mySlot = -1; // --- Will there be another attempt to find a slot?
    }
    else {
        EV << "Master: SUCCESS: New slot is : " << mySlot << endl;
    }
    slotChange->recordWithTimestamp(simTime(), FindModule<>::findHost(this)->getId() - 4);
}

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
        EV_DETAIL << "packet put in queue\n  queue size: " << macQueue.size() << " macState: " << macState
                  << "; mySlot is " << mySlot << "; current slot is " << currSlot << endl;
        ;
    }
    else {
        // queue is full, message has to be deleted
        EV_DETAIL << "New packet arrived, but queue is FULL, so new packet is deleted\n";
        delete mac;
        EV_DETAIL << "WARNING: Queue is full, forced to delete packet.\n";
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
    if (signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
            // if data is scheduled for transfer, don;t do anything.
            if (sendData->isScheduled()) {
                EV_DETAIL << "Signal: transmission of control packet over. data transfer will start soon." << endl;
            }
            else {
                EV_DETAIL << "Signal: transmission over. nothing else is scheduled, get back to sleep." << endl;
                //
                // --- Go back to sleep, nothing more to TX
                //
                macState = SLEEP;

                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

                EV_DETAIL << "Old state: ?, New state: SLEEP" << endl;

                if (timeout->isScheduled())
                    cancelEvent(timeout);
            }
        }
        transmissionState = newRadioTransmissionState; // --- Should be RADIO_MODE_SLEEP?
    }
    else if (signalID == IRadio::radioModeChangedSignal) {
        IRadio::RadioMode radioMode = (IRadio::RadioMode)value;
        if (macState == SEND_CONTROL && radioMode == IRadio::RADIO_MODE_TRANSMITTER) {
            // we just switched to TX after CCA, so simply send the first sendPremable self message
            EV_DETAIL << "SIgnal: radio set to transmitter mode sending 'send_control' message." << endl;
            scheduleAt(simTime(), send_control);
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
    // delete the macPkt
    delete msg;
    EV_DETAIL << " message decapsulated " << endl;
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

