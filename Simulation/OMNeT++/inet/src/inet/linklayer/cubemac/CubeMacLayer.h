#ifndef __INET_CUBEMACLAYER_H
#define __INET_CUBEMACLAYER_H

#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "inet/linklayer/contract/IMACProtocol.h"
#include "inet/linklayer/common/MACAddress.h"
#include "inet/linklayer/base/MACProtocolBase.h"
#include "inet/linklayer/cubemac/CubeMacFrame_m.h"

namespace inet {

using namespace physicallayer;

/**
 * @brief LMAC Base
 *
 * During the first 5 full frames nodes will be waking up every controlDuration
 * to setup the network first by assigning a different timeslot to each node.
 * Normal packets will be queued, but will be send only after the setup phase.
 *
 * During its timeslot a node wakes up, checks the channel for a short random
 * period (CCA) to check for possible collision in the slot and, if the
 * channel is free, sends a control packet. If there is a collision it tries
 * to change its timeslot to an empty one. After the transmission of the control
 * packet it checks its packet queue and if its non-empty it sends a data
 * packet.
 *
 * During a foreign timeslot a node wakes up, checks the channel for
 * 2*controlDuration period for an incoming control packet and if there in
 * nothing it goes back to sleep and conserves energy for the rest of the
 * timeslot. If it receives a control packet addressed for itself it stays awake
 * for the rest of the timeslot to receive the incoming data packet.
 *
 * @ingroup macLayer
 **/

class INET_API CubeMacLayer : public MACProtocolBase, public IMACProtocol
{
  private:
    /** @brief Copy constructor is not allowed.*/
    CubeMacLayer(const CubeMacLayer&);
    /** @brief Assignment operator is not allowed. */
    CubeMacLayer& operator=(const CubeMacLayer&);

  public:
    CubeMacLayer()
        : MACProtocolBase()
        // --- Added --- //
        , startTime(0.0)
        , myId(0)
        , myClusterId(0)
        , masterId(0)
        , slaveId(0)
        , isSlave(false)
        , slavesInCluster(0)
        // --- Added --- //
        , slotChange()
        , macState()
        , slotDuration(0)
        , headerLength(0)
        , controlDuration(0)
        , mySlot(0)
        , numSlots(0)
        , currSlot()
        , macQueue()
        , radio(nullptr)
        , transmissionState(IRadio::TRANSMISSION_STATE_UNDEFINED)
        , queueLength(0)
        , wakeUp(nullptr)
        , timeout(nullptr)
        , sendData(nullptr)
        , checkChannel(nullptr)
        , startCubemac(nullptr)
        , sendControl(nullptr)
        , bitrate(0)
    {} // --- What do you do?
    
    /** @brief Clean up messges.*/
    virtual ~CubeMacLayer();

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int) override;

    /** @brief Handle messages from lower layer */
    virtual void handleLowerPacket(cPacket *) override;

    /** @brief Handle messages from upper layer */
    virtual void handleUpperPacket(cPacket *) override;

    /** @brief Handle self messages such as timers */
    virtual void handleSelfMessage(cMessage *) override;

    /** @brief Handle control messages from lower layer */
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, long value DETAILS_ARG) override;

    /** @brief Encapsulate the NetwPkt into an MacPkt */
    virtual CubeMacFrame *encapsMsg(cPacket *);
    virtual cPacket *decapsMsg(CubeMacFrame *);
    cObject *setUpControlInfo(cMessage *const pMsg, const MACAddress& pSrcAddr);

  protected:
    /** @brief Generate new interface address*/
    virtual void initializeMACAddress();
    virtual InterfaceEntry *createInterfaceEntry() override;
    virtual void handleCommand(cMessage *msg) {}

    typedef std::list<CubeMacFrame *> MacQueue;

    /** @brief MAC states
     *
     *  The MAC states help to keep track what the MAC is actually
     *  trying to do -- this is esp. useful when radio switching takes
     *  some time.
     *  SLEEP -- the node sleeps but accepts packets from the network layer
     *  RX  -- MAC accepts packets from PHY layer
     *  TX  -- MAC transmits a packet
     *  CCA -- Clear Channel Assessment - MAC checks whether medium is busy
     *
     */

    enum States {
        INIT, SLEEP, CCA, WAIT_CONTROL, WAIT_DATA, SEND_CONTROL, SEND_DATA, WAIT_SLAVE_CONTROL , WAIT_SLAVE_DATA
    };

    enum TYPES {
        CUBEMAC_CONTROL = 167,
        CUBEMAC_TIMEOUT = 168,
        CUBEMAC_WAKEUP = 169,
        CUBEMAC_SEND_DATA = 170,
//        CUBEMAC_SETUP_PHASE_END = 171,
        CUBEMAC_CHECK_CHANNEL = 172,
        CUBEMAC_SOMEBODY = 173,
        CUBEMAC_DATA = 174,
        CUBEMAC_START_CUBEMAC = 175,
        CUBEMAC_SEND_CONTROL = 176,
        CUBEMAC_SLAVE_CONTROL = 177, // --- Added --- Pray these don't brake something odd
        CUBEMAC_SLAVE_DATA = 178 // --- Added
    };

    //
    // --- Added
    //
    simtime_t startTime;

    int myId;
    int myClusterId;

    int masterId;
    int slaveId;

    bool isSlave;

    // --- Used by masters to collect multiple slave packets during uplink
    int slavesInCluster;
    int expectedSlaveControlPackets;
    // ---

    /** @brief dummy receiver address to indicate no pending packets in the control packet */
    static const MACAddress CUBEMAC_NO_RECEIVER;
    static const MACAddress CUBEMAC_FREE_SLOT;
    // --- Added
    static const MACAddress CUBEMAC_BROADCAST;

    /** @brief the setup phase is the beginning of the simulation, where only control packets at very small slot durations are exchanged. */
    bool SETUP_PHASE;

    /** @brief indicate how often the node needs to change its slot because of collisions */
    cOutVector *slotChange;

    /** @brief keep track of MAC state */
    States macState;

    /** @brief Duration of a slot */
    double slotDuration;

    /** @brief Length of the header*/
    int headerLength;

    /** @brief Duration of the control time in each slot */
    double controlDuration;

    /** @brief my slot ID */
    int mySlot;

    /** @brief how many slots are there */
    int numSlots;

    /** @brief The current slot of the simulation */
    int currSlot;

    /** @brief The MAC address of the interface. */
    MACAddress address;

    /** @brief A queue to store packets from upper layer in case another
       packet is still waiting for transmission..*/
    MacQueue macQueue;

    /** @brief The radio. */
    IRadio *radio;
    IRadio::TransmissionState transmissionState;

    /** @brief length of the queue*/
    unsigned queueLength;

    cMessage *wakeUp;
    cMessage *timeout;
    cMessage *sendData;
    cMessage *checkChannel;
    cMessage *startCubemac;
    cMessage *sendControl;

    /** @brief the bit rate at which we transmit */
    double bitrate;

    /** @brief Internal function to attach a signal to the packet */
    void attachSignal(CubeMacFrame *macPkt);

    virtual void flushQueue();

    virtual void clearQueue();
};

} // namespace inet

#endif // ifndef __INET_CUBEMACLAYER_H

