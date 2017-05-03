#include <limits>
#include <vector>

#include "inet/common/ModuleAccess.h"
#include "inet/common/geometry/common/coord.h"

#include "inet/mobility/single/CircleMobility.h"

#include "inet/power/storage/IdealEnergyStorage.h"
#include "inet/common/Units.h"

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

            // TODO: Differing metrics for election
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

    Coord groundCoord = Coord(0,0,0);

    // Get position of ground
    CircleMobility *gcm = check_and_cast<CircleMobility*>(sim->getModuleByPath("nodeGround[0].mobility"));

    if (gcm != nullptr)
        groundCoord = gcm->getCurrentPosition();
    else
        throw cRuntimeError("Unable to cast ground station circle mobility submodule");

    if (groundCoord == Coord(0,0,0))
        throw cRuntimeError("Unable to get coordinates for ground station");

    dymo::DYMO *currentGroundMaster = nullptr;
    dymo::DYMO *closestMaster = nullptr;

    double min_dist = std::numeric_limits<double>::max();

    std::map<int, double> masterEnergies;

    for (SubmoduleIterator i = SubmoduleIterator(sim); !i.end(); i++){
        cModule *m = i.operator *();

        if (m->isName("nodeMaster")){
            EV_DETAIL << "Assessing the role of: " << m->getFullName() << endl;

            int masterIndex = m->getIndex();

            dymo::DYMO *d = check_and_cast<dymo::DYMO*>(m->getSubmodule("dymo"));
            if (d != nullptr) {
                if (d->getIsGroundMaster())
                    currentGroundMaster = d;

                power::IdealEnergyStorage *mes = check_and_cast<power::IdealEnergyStorage*>(m->getSubmodule("energyStorage"));

                if (mes != nullptr) {
                    units::value<double, units::units::J> pc = mes->getEnergyBalance();
                    double pcd = pc.get();
                    masterEnergies[masterIndex] = pcd;
                }
                else
                    throw cRuntimeError("Unable to cast master energy storage submodule");

                CircleMobility *mcm = check_and_cast<CircleMobility*>(m->getSubmodule("mobility"));

                if (mcm != nullptr) {
                    Coord masterCoord = Coord(0,0,0);
                    masterCoord = mcm->getCurrentPosition();

                    if (masterCoord == Coord(0,0,0))
                        throw cRuntimeError("Unable to get coordinates for master");

                    double distanceFromGround = masterCoord.distance(groundCoord);

                    if (distanceFromGround < min_dist){
                        min_dist = distanceFromGround;
                        closestMaster = d;
                    }
                }
                else
                    throw cRuntimeError("Unable to cast master circle mobility submodule");

            }
            else
                throw cRuntimeError("Unable to cast dymo submodule");
        }
    }

    // Iterate through all elements in std::map
    std::map<int, double>::iterator it = masterEnergies.begin();
    while(it != masterEnergies.end())
    {
        EV_DETAIL << "Master[" << it->first <<"] energy balance " << it->second << "J" << endl;
        it++;
    }

    if (currentGroundMaster == nullptr)
        throw cRuntimeError("Unable to establish current ground master");

    else if (closestMaster == nullptr)
        throw cRuntimeError("Unable to establish which master is closest to ground");

    else if (currentGroundMaster != closestMaster){
        EV_DETAIL << "currentGroundMaster != closestMaster: Attempting role changes" << endl;
        currentGroundMaster->unsetGroundMaster();
        closestMaster->setGroundMaster();
        deleteGroundRoutes();
    }

    else
        EV_DETAIL << "currentGroundMaster == closestMaster: Roles unchanged" << endl;

    //    cModuleType *moduleType = cModuleType::get("inet.node.dymo.DYMORouter");
    //    EV_DETAIL << "DYMORouter type info: " << moduleType->info() << endl;
}

void RoleOracle::deleteGroundRoutes(){
    const cModule *sim = getParentModule();

    for (SubmoduleIterator i = SubmoduleIterator(sim); !i.end(); i++){
        cModule *m = i.operator *();

        if (m->isName("nodeMaster") || m->isName("nodeSlave")){
            EV_DETAIL << "Deleting ground routes for: " << m->getFullName() << endl;

            dymo::DYMO *d = check_and_cast<dymo::DYMO*>(m->getSubmodule("dymo"));
            if (d != nullptr) {
                d->deleteGroundRoute();
            }
            else
                throw cRuntimeError("Unable to cast dymo submodule");
        }
    }

}

} // namespace roleoracle

} // namespace inet

