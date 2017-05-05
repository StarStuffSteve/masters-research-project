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
        targetMaster = par("targetMaster");

        useEnergies = par("useEnergies");
        energyRankWeight = par("energyRankWeight");

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
    double minDistance = std::numeric_limits<double>::max();

    std::map<int, double> masterDistances;
    std::map<int, double> masterEnergies;

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
                }
                else
                    throw cRuntimeError("Unable to cast master energy storage submodule");

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
                    masterDistances[masterIndex] = distanceFromGround;

                    if (distanceFromGround < minDistance){
                        minDistance = distanceFromGround;
                        closestMaster = d;
                    }
                }
                else
                    throw cRuntimeError("Unable to cast master linear mobility submodule");
            }
            else
                throw cRuntimeError("Unable to cast dymo submodule");
        }
    }

    if (currentGroundMaster == nullptr)
        throw cRuntimeError("Unable to establish current ground master");

    //
    // DISTANCE & ENERGY MASTER SELECTION
    //

    if (useEnergies)
    {
        EV_DETAIL << "Using energy rank weight: " << energyRankWeight << endl;

        double lowestScore = std::numeric_limits<double>::max();
        double lowestScoreMasterDistance = -1.0;
        dymo::DYMO *lowestScoreMaster = nullptr;

        std::map<int, double>::iterator it1 = masterEnergies.begin();
        while(it1 != masterEnergies.end())
        {
            int energyRank = 1;// The lower the better

            // Get a rank for master[it->first]
            std::map<int, double>::iterator it2 = masterEnergies.begin();
            while(it2 != masterEnergies.end())
            {
                // If master[it1->first] has consumed more energy than master[it2->first]
                if(it1 != it2 && it1->second < it2->second)
                    energyRank++;

                it2++;
            }

            // The effective distance of a master increase in relation to it's rank
            double score = masterDistances[it1->first] + (masterDistances[it1->first] * (energyRank * energyRankWeight));

            if (score < lowestScore) {
                lowestScore = score;
                lowestScoreMaster = masterDymoPointers[it1->first];
                lowestScoreMasterDistance = masterDistances[it1->first];
            }

            EV_DETAIL << masterDymoPointers[it1->first]->getParentModule()->getFullName()
                      <<" Energy balance " << it1->second << "J"
                      <<" Energy rank: " << energyRank
                      <<" Distance from ground: " << masterDistances[it1->first]
                      <<" Score: " << score << endl;

            it1++;
        }

        EV_DETAIL << "Master with lowest score: " << lowestScoreMaster->getParentModule()->getFullName() << endl;

        ASSERT(lowestScoreMasterDistance >= 1);
        ASSERT(lowestScoreMaster != nullptr);

        // What happens on wrap around?
        if (currentGroundMaster != lowestScoreMaster && lowestScoreMasterDistance < 125.0) {
            EV_DETAIL << "currentGroundMaster != lowestScoreMaster: Attempting role change" << endl;

            currentGroundMaster->unsetGroundMaster();
            lowestScoreMaster->setGroundMaster();

            deleteGroundRoutes();
        }
    }

    //
    // DISTANCE ONLY MASTER SELECTION
    //

    else {
        if (closestMaster == nullptr)
            throw cRuntimeError("Unable to establish which master is closest to ground");

        else if (currentGroundMaster != closestMaster){
            EV_DETAIL << "currentGroundMaster != closestMaster: Attempting role changes" << endl;

            currentGroundMaster->unsetGroundMaster();
            closestMaster->setGroundMaster();

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

