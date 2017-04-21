#include "inet/common/ModuleAccess.h"

#include "inet/oracle/common/RoleOracle.h"

namespace inet {

namespace roleoracle {

Define_Module(RoleOracle);

RoleOracle::RoleOracle() :
    updateFrequency(NaN),
    updateTimer(nullptr)
{
}

RoleOracle::~RoleOracle()
{
    cancelAndDelete(updateTimer);
}

void RoleOracle::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
    {
        updateFrequency = par("updateFrequency").doubleValue();

        updateTimer = new cMessage("updateTimer");
        updateTimer->setKind(ORACLE_UPDATE_TIMER);

        scheduleAt(updateFrequency, updateTimer);
    }
}

void RoleOracle::handleMessage(cMessage *msg){
    if (msg->isSelfMessage()){
        if (msg->getKind() == ORACLE_UPDATE_TIMER){
            EV_DETAIL << "Update Timer" << endl;
        }
        else
            throw cRuntimeError("0");
    }
    else
        throw cRuntimeError("1");

    if (updateTimer->isScheduled())
        cancelEvent(updateTimer);

    scheduleAt(simTime() + updateFrequency, updateTimer);
}

} // namespace roleoracle

} // namespace inet

