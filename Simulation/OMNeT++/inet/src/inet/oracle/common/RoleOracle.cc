#include "inet/common/ModuleAccess.h"

#include "inet/oracle/common/RoleOracle.h"

#include "inet/routing/dymo/DYMO.h"

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
            EV_DETAIL << "Handling update timer" << endl;

            updateRoles();
        }
        else
            throw cRuntimeError("Unknown self-message");
    }
    else
        throw cRuntimeError("Unknown message not from self");

    if (updateTimer->isScheduled())
        cancelEvent(updateTimer);

    scheduleAt(simTime() + updateFrequency, updateTimer);
}

void RoleOracle::updateRoles(){
    const cModule *sim = getParentModule();

    cModuleType *moduleType = cModuleType::get("inet.node.dymo.DYMORouter");
    EV_DETAIL << "DYMORouter type info: " << moduleType->info() << endl;


    for (SubmoduleIterator i = SubmoduleIterator(sim); !i.end(); i++){
        cModule *m = i.operator *();

        if (m->isName("nodeMaster")){
            inet::dymo::DYMO *d = check_and_cast<inet::dymo::DYMO*>(m->getSubmodule("dymo"));

            if (d != nullptr) {
                EV_DETAIL << "Module: " << m->getFullName() << endl;
                EV_DETAIL << "\tisGroundMaster: " << d->getIsGroundMaster() << endl;
            }
            else
                throw cRuntimeError("Unable to cast dymo sudmodule");
        }
    }
}

} // namespace roleoracle

} // namespace inet

