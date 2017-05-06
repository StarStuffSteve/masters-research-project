#ifndef __INET_CUBEMACLAYER_H
#define __INET_CUBEMACLAYER_H

#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "inet/linklayer/contract/IMACProtocol.h"
#include "inet/linklayer/common/MACAddress.h"
#include "inet/linklayer/base/MACProtocolBase.h"
#include "inet/linklayer/cubemac/CubeMacFrame_m.h"

namespace inet {

using namespace physicallayer;

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
        , myClusterId(0)
        , isSlave(false)
        , isGround(false)
        , slavesInCluster(0)
        , uplinkSlot(0)
        , timeoutDuration(0.01)
        , slotPadding(0.01)
        , energySavingFeatures(true)
        // --- Results =, Stats etc.
        , packetsOnQueue(0)
        , pureTDMA(false)
        // --- Added --- //
        , macState()
        , slotDuration(0)
        , headerLength(0)
        , mySlot(0)
        , numSlots(0)
        , currSlot()
        , macQueue()
        , radio(nullptr)
        , transmissionState(IRadio::TRANSMISSION_STATE_UNDEFINED)
        , queueLength(0)
        , startCubemac(nullptr)
        , wakeUp(nullptr)
        , timeout(nullptr)
        , sendData(nullptr)
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

    enum States {
        INIT, SLEEP, SEND_DATA, WAIT_MASTER_DATA, WAIT_SLAVE_DATA
    };

    enum TYPES {
        CUBEMAC_START_CUBEMAC = 167, // Self
        CUBEMAC_WAKEUP = 168, // Self
        CUBEMAC_TIMEOUT = 169, // Self
        CUBEMAC_SEND_DATA = 170, // Self
        CUBEMAC_MASTER_DATA = 171,
        CUBEMAC_SLAVE_DATA = 172
    };

    //
    // --- Added
    //
    // TODO: Comments
    simtime_t startTime;

    int myClusterId;

    bool isSlave;

    bool isGround;

    int slavesInCluster;
    int expectedSlaveDataPackets;

    int uplinkSlot;

    double timeoutDuration;

    simtime_t slotPadding; // Used to pad end of slot when sending multiple packets in slot

    simtime_t currentSlotEndTime;
    bool canSendNextPacket;

    bool energySavingFeatures;

    // --- Results, Stats, Watched etc.
    int packetsOnQueue;
    cOutVector accessDelayMAC;

    // ---

    bool pureTDMA;

    static const MACAddress CUBEMAC_BROADCAST;

    /** @brief keep track of MAC state */
    States macState;

    /** @brief Duration of a slot */
    double slotDuration;

    /** @brief Length of the header*/
    int headerLength;

    /** @brief my slot ID */
    int mySlot;

    /** @brief how many slots are there */
    int numSlots;

    /** @brief The current slot of the simulation */
    int currSlot;

    /** @brief The MAC address of the interface. */
    MACAddress myAddress;

    /** @brief A queue to store packets from upper layer in case another
       packet is still waiting for transmission..*/
    MacQueue macQueue;

    /** @brief The radio. */
    IRadio *radio;
    IRadio::TransmissionState transmissionState;

    /** @brief length of the queue*/
    unsigned queueLength;

    cMessage *startCubemac;
    cMessage *wakeUp;
    cMessage *timeout;
    cMessage *sendData;

    /** @brief the bit rate at which we transmit */
    double bitrate;

    /** @brief Internal function to attach a signal to the packet */
    void attachSignal(CubeMacFrame *macPkt);

    virtual void flushQueue();

    virtual void clearQueue();
};

} // namespace inet

#endif // ifndef __INET_CUBEMACLAYER_H

