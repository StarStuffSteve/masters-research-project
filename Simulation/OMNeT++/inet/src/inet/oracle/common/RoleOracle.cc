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

simsignal_t RoleOracle::groundMasterChangedSignal = registerSignal("groundMasterChanged");

RoleOracle::RoleOracle() :
    updateFrequency(NaN),
    updateTimer(nullptr),
    hysteresis(10),
    rolesChanged(true),
    useEnergies(false),
    energyRankWeight(1.0),
    groundMasterCooldown(200.0),
    enforceSouthOfGround(false),
    useFixedTimes(false),
    useEnergyBudgets(false),
    maxGroundMasterDuration(54.0),
    energyBudget(-0.0001)
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

        hysteresis = par("hysteresis");

        useEnergies = par("useEnergies");
        energyRankWeight = par("energyRankWeight");

        groundMasterCooldown = par("groundMasterCooldown");
        enforceSouthOfGround = par("enforceSouthOfGround");
        useFixedTimes = par("useFixedTimes");
        useEnergyBudgets = par("useEnergyBudgets");

        maxGroundMasterDuration = par("maxGroundMasterDuration");
        energyBudget = par("energyBudget");

        // For fixed-times (crude 'get it working' hack)
        nextGM[212] = 110;
        nextGM[263] = 161;
        nextGM[59] = 212;
        nextGM[110] = 263;
        nextGM[161] = 59;

        emit(groundMasterChangedSignal, 212);

        scheduleAt(simTime(), updateTimer);
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

    if(rolesChanged) {
        rolesChanged = false;
        // Note the hysteresis parameter being applied to the updateFrequeny
        scheduleAt(simTime() + updateFrequency*hysteresis, updateTimer);
    }
    else
        scheduleAt(simTime() + updateFrequency, updateTimer);
}

void RoleOracle::updateRoles(){
    const cModule *sim = getParentModule();

    // Get position of ground
    Coord groundCoord = Coord(0,0,0);
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

    // ---
    // Collect info on masters
    // ---
    for (SubmoduleIterator i = SubmoduleIterator(sim); !i.end(); i++){
        cModule *m = i.operator *();

        if (m->isName("nodeMaster")){
            // Get DYMO module for this master
            dymo::DYMO *d = check_and_cast<dymo::DYMO*>(m->getSubmodule("dymo"));
            int masterId = d->getId();

            if (d != nullptr) {
                masterDymoPointers[masterId] = d;

                if (d->getIsGroundMaster()){
                    EV_DETAIL << "Current ground master: " << d->getParentModule()->getFullName() << endl;
                    currentGroundMaster = d;
                }

                // ---
                // Collect energies
                // ---
                if (useEnergies) {
                    power::IdealEnergyStorage *mes = check_and_cast<power::IdealEnergyStorage*>(m->getSubmodule("energyStorage"));

                    if (mes != nullptr) {
                        units::value<double, units::units::J> pc = mes->getEnergyBalance();
                        double pcd = pc.get();

                        masterEnergies[masterId] = pcd;
                    }
                    else
                        throw cRuntimeError("Unable to cast master energy storage submodule");
                }

                // ---
                // Collect distances
                // ---
                LinearMobility *mlm = check_and_cast<LinearMobility*>(m->getSubmodule("mobility"));

                if (mlm != nullptr)
                {
                    Coord masterCoord = Coord(0,0,0);
                    masterCoord = mlm->getCurrentPosition();

                    if (masterCoord == Coord(0,0,0))
                        throw cRuntimeError("Unable to get coordinates for master");

                    double distanceFromGround = masterCoord.distance(groundCoord);

                    if (distanceFromGround < minDistance){
                        minDistance = distanceFromGround;
                        closestMaster = d;
                    }

                    masterDistances[masterId] = distanceFromGround;
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

    // ---
    // Energy-distance
    // ---
    if (useEnergies)
    {
        double lowestScore = std::numeric_limits<double>::max();
        double lowestScoreMasterDistance = -1.0;
        bool southOfGround = true; // only changes if enforceSouthOfGround == True
        dymo::DYMO *lowestScoreMaster = nullptr;

        std::map<int, double>::iterator it1 = masterEnergies.begin();
        // NB: it1->first is the index of the current master of interest
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

            // Enforce south of ground
            if (enforceSouthOfGround){
                // Hope this works ...
                LinearMobility *mlm = check_and_cast<LinearMobility*>(masterDymoPointers[it1->first]->getParentModule()->getSubmodule("mobility"));
                Coord currCoord = mlm->getCurrentPosition();

                // If currCoord is North of ground
                if (currCoord.y <= groundCoord.y)
                    southOfGround = false;
                else
                    southOfGround = true;
            }

            // The effective distance of a master increase in relation to it's rank
            double score = masterDistances[it1->first] + (masterDistances[it1->first] * (energyRank * energyRankWeight));

            if (score < lowestScore && southOfGround) {
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
        } // END: while

        if (lowestScoreMaster != nullptr) {
            EV_DETAIL << "Master with lowest score: " << lowestScoreMaster->getParentModule()->getFullName() << endl;

            // Will only change to a master which
            double lastElected = lowestScoreMaster->getLastTimeElected(); // Recorded as GM *start* time

            double timeSinceElected = groundMasterCooldown; // Master is eligible if lastElected == -1
            if (lastElected != -1.0){
                timeSinceElected = (simTime().dbl()) - lastElected;

                if (timeSinceElected < 0)
                    throw cRuntimeError("timeSinceElected is negative");

                if (lastElected > simTime().dbl())
                    throw cRuntimeError("lastElected > simTime()");
            }

            // Don't re-elect current GM
            // Only elect masters which are within range of ground
            // Only elect masters which have not had the GM role for at least groundMasterCooldown
            // TODO: Should we elect 'next best' master if the 'best' is not suitable?
            if (currentGroundMaster != lowestScoreMaster &&
                    lowestScoreMasterDistance < 125.0 &&
                    timeSinceElected >= groundMasterCooldown) {
                EV_DETAIL << "currentGroundMaster != lowestScoreMaster: Executing role change" << endl;

                currentGroundMaster->unsetGroundMaster();
                lowestScoreMaster->setGroundMaster();

                emit(groundMasterChangedSignal, (lowestScoreMaster->getId()));
                rolesChanged = true;
                deleteGroundRoutes();
            }
        }
        else {
            if (enforceSouthOfGround)
                EV_DETAIL << "enforceSouthOfGround: No appropriate lowestScoreMaster available" << endl;
            else
                throw cRuntimeError("Failed to identify lowestScoreMaster");
        }
    }

    // ---
    // Fixed time per master
    // ---
    else if (useFixedTimes) {

    }

//    // ---
//    // Energy budget
//    // ---
//    else if (useEnergyBudget) {
//
//    }
    // ---
    // Closest-master
    // ---
    else {
        if (closestMaster == nullptr)
            throw cRuntimeError("Unable to establish which master is closest to ground");

        else if (currentGroundMaster != closestMaster){
            EV_DETAIL << "currentGroundMaster != closestMaster: Attempting role changes" << endl;

            currentGroundMaster->unsetGroundMaster();
            closestMaster->setGroundMaster();

            emit(groundMasterChangedSignal, (closestMaster->getId()));
            rolesChanged = true;
            deleteGroundRoutes();
        }

        else
            EV_DETAIL << "currentGroundMaster == closestMaster: Roles unchanged" << endl;
    }

} // END: HandleUpdateRoles

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

} // END: namespace roleoracle

} // END: namespace inet

