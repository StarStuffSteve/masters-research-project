#include <limits>
#include <vector>

#include "inet/common/ModuleAccess.h"
#include "inet/common/geometry/common/coord.h"

#include "inet/mobility/single/LinearMobility.h"

#include "inet/power/storage/IdealEnergyStorage.h"
#include "inet/common/Units.h"

#include "inet/oracle/common/RoleOracle.h"

#include "inet/routing/dymo/DYMO.h"

namespace inet {

namespace roleoracle {

Define_Module(RoleOracle);

RoleOracle::RoleOracle() :
    updateFrequency(NaN),
    updateTimer(nullptr),
    overloadMaster(false),
    hysteresis(10),
    rolesChanged(true),
    targetMaster(1),
    useEnergies(false),
    energyRankWeight(1.0)
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

        overloadMaster = par("overloadMaster");
        hysteresis = par("hysteresis");
        targetMaster = par("targetMaster");

        useEnergies = par("useEnergies");
        energyRankWeight = par("energyRankWeight");

        scheduleAt(simTime(), updateTimer);
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

    if(rolesChanged) {
        rolesChanged = false;
//        scheduleAt(simTime() + updateFrequency*hysteresis, updateTimer);
        scheduleAt(simTime() + 30.0, updateTimer);
    }
    else
        scheduleAt(simTime() + updateFrequency, updateTimer);
}

void RoleOracle::updateRoles(){
    const cModule *sim = getParentModule();

    Coord groundCoord = Coord(0,0,0);

    // Get position of ground
    LinearMobility *glm = check_and_cast<LinearMobility*>(sim->getModuleByPath("nodeGround[0].mobility"));

    if (glm != nullptr)
        groundCoord = glm->getCurrentPosition();
    else
        throw cRuntimeError("Unable to cast ground station linear mobility submodule");

    if (groundCoord == Coord(0,0,0))
        throw cRuntimeError("Unable to get coordinates for ground station");

    // ---

    dymo::DYMO *currentGroundMaster = nullptr;
    dymo::DYMO *closestMaster = nullptr;
    dymo::DYMO *nextMaster = nullptr;
    double minDistance = std::numeric_limits<double>::max();
    double nextMinDistance = std::numeric_limits<double>::max();

    std::map<int, double> masterDistances;
    std::map<int, double> masterEnergies;

    double currentGroundMasterEnergy = std::numeric_limits<int>::min();
    double closestMasterEnergy = std::numeric_limits<int>::min();
    double nextMasterEnergy = std::numeric_limits<int>::min();

    double currentGroundMasterDistance = std::numeric_limits<double>::max();;

    double totalLastPassEnergy = 0.0;
    int lastPassCount = 0;

    double totalEnergy = 0.0;
    int energyCount = 0;

    std::map<int, dymo::DYMO*> masterDymoPointers;

    //
    // COLLECTING INFO ON MASTERS
    //

    for (SubmoduleIterator i = SubmoduleIterator(sim); !i.end(); i++){
        cModule *m = i.operator *();

        if (m->isName("nodeMaster")){
//            EV_DETAIL << "Assessing the role of: " << m->getFullName() << endl;

            int masterIndex = m->getIndex();

            dymo::DYMO *d = check_and_cast<dymo::DYMO*>(m->getSubmodule("dymo"));
            if (d != nullptr) {
                masterDymoPointers[masterIndex] = d;

                if (d->getIsGroundMaster()){
                    EV_DETAIL << "Current ground master: " << d->getParentModule()->getFullName() << endl;
                    currentGroundMaster = d;
                }

                //
                // ENERGIES
                //
                if (useEnergies) {
                    power::IdealEnergyStorage *mes = check_and_cast<power::IdealEnergyStorage*>(m->getSubmodule("energyStorage"));

                    if (mes != nullptr) {
                        units::value<double, units::units::J> pc = mes->getEnergyBalance();
                        double pcd = pc.get();

                        masterEnergies[masterIndex] = pcd;

                        // Will only be performed on first update
                        if (overloadMaster && masterIndex == targetMaster && simTime() < (2*updateFrequency)){
                            EV_DETAIL << "Overloading master: " << targetMaster << endl;
                            mes->updateEnergyBalance(10.0);
                        }

                        if (d->getIsGroundMaster()){
                            currentGroundMasterEnergy = pcd;
                            totalEnergy += d->getLastGmStartEnergy();
                        }
                        else {
                            totalEnergy += pcd;
                        }

                        energyCount++;

                        double energy;

                        if ((energy = d->getLastGmEnergyCost()) != 0) {
                            totalLastPassEnergy += energy;
                            lastPassCount++;
                        }
                    }
                    else
                        throw cRuntimeError("Unable to cast master energy storage submodule");
                }

                //
                // DISTANCES
                //

                LinearMobility *mlm = check_and_cast<LinearMobility*>(m->getSubmodule("mobility"));

                if (mlm != nullptr)
                {
                    Coord masterCoord = Coord(0,0,0);
                    masterCoord = mlm->getCurrentPosition();

                    if (masterCoord == Coord(0,0,0))
                        throw cRuntimeError("Unable to get coordinates for master");

                    double distanceFromGround = masterCoord.distance(groundCoord);

                    if (d->getIsGroundMaster()){
                        currentGroundMasterDistance = distanceFromGround;
                    }

                    if (distanceFromGround < minDistance){
                        minDistance = distanceFromGround;
                        closestMaster = d;
                        closestMasterEnergy = masterEnergies[masterIndex];
                    }

                    if (distanceFromGround < nextMinDistance && simTime() - d->getLastGmTime() > SimTime(200, SIMTIME_S)) {
                        nextMinDistance = distanceFromGround;
                        nextMaster = d;
                        nextMasterEnergy = masterEnergies[masterIndex];;
                    }

                    masterDistances[masterIndex] = distanceFromGround;
                }
                else
                    throw cRuntimeError("Unable to cast master linear mobility submodule");
            }
            else
                throw cRuntimeError("Unable to cast dymo submodule");
        }
    }

//    if (currentGroundMaster == nullptr)
//        throw cRuntimeError("Unable to establish current ground master");



    //
    // DISTANCE & ENERGY MASTER SELECTION
    //

    if (useEnergies)
    {
        double avgGroundMasterCost = 0.0;
        double avgMasterEnergy = 0.0;
        
        if (lastPassCount > 3) {
            avgGroundMasterCost = totalLastPassEnergy / lastPassCount;
        }
        
        if (energyCount > 3) {
            avgMasterEnergy = totalEnergy / energyCount;
        }

        if (closestMaster == nullptr)
            throw cRuntimeError("Unable to establish which master is closest to ground");

        else if (currentGroundMaster == nullptr) {
            closestMaster->setGroundMaster();
            closestMaster->recordGmStartEnergy(closestMasterEnergy);
            rolesChanged = true;
            deleteGroundRoutes();
        }

        else if (currentGroundMaster != nextMaster) {

            if (avgGroundMasterCost != 0.0
                    && currentGroundMasterEnergy > avgMasterEnergy - avgGroundMasterCost
                    && currentGroundMasterDistance < 125) {

                EV_DETAIL << "currentGroundMaster != nextMaster BUT currentGroundMaster has not depleted its energy budget and still in range - roles unchanged" << endl;

            }
            else {
                EV_DETAIL << "currentGroundMaster != nextMaster AND currentGroundMaster has depleted its energy budget OR falling back to closest master - attempting role change" << endl;

                currentGroundMaster->unsetGroundMaster();
                currentGroundMaster->recordGmEndEnergy(currentGroundMasterEnergy);
                nextMaster->setGroundMaster();
                nextMaster->recordGmStartEnergy(nextMasterEnergy);

                rolesChanged = true;
                deleteGroundRoutes();
            }
        }

        else {

            // currentGroundMaster == closestMaster


            if (avgGroundMasterCost != 0.0
                    && currentGroundMasterEnergy < avgMasterEnergy - avgGroundMasterCost) {

                EV_DETAIL << "currentGroundMaster == closestMaster but energy this pass depleted - attempting role change" << endl;

                currentGroundMaster->unsetGroundMaster();
                currentGroundMaster->recordGmEndEnergy(currentGroundMasterEnergy);
                nextMaster->setGroundMaster();
                nextMaster->recordGmStartEnergy(nextMasterEnergy);

                rolesChanged = true;
                deleteGroundRoutes();

            } else {

                EV_DETAIL << "currentGroundMaster == closestMaster: Falling back to closest master - roles unchanged" << endl;

            }

        }

    }

    //
    // DISTANCE ONLY MASTER SELECTION
    //

    else {
        if (closestMaster == nullptr)
            throw cRuntimeError("Unable to establish which master is closest to ground");

        else if (currentGroundMaster == nullptr) {
            closestMaster->setGroundMaster();
            rolesChanged = true;
            deleteGroundRoutes();
        }

        else if (currentGroundMaster != closestMaster){
            EV_DETAIL << "currentGroundMaster != closestMaster: Attempting role changes" << endl;

            currentGroundMaster->unsetGroundMaster();
            closestMaster->setGroundMaster();

            rolesChanged = true;
            deleteGroundRoutes();
        }

        else
            EV_DETAIL << "currentGroundMaster == closestMaster: Roles unchanged" << endl;
    }

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

