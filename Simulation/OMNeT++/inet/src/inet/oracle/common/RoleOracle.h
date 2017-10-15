#ifndef __INET_ROLEORACLE_H
#define __INET_ROLEORACLE_H

#include "inet/oracle/contract/IRoleOracle.h"

namespace inet {

namespace roleoracle {

class INET_API RoleOracle : public cSimpleModule, public IRoleOracle
{
  protected:
    virtual void initialize(int stage) override;

    simtime_t updateFrequency;
    cMessage *updateTimer;

    int hysteresis;
    bool rolesChanged;

    bool useEnergies;
    double energyRankWeight;

    enum TYPES {
        ORACLE_UPDATE_TIMER = 303
    };

    static simsignal_t groundMasterChangedSignal;

    void updateRoles();
    void deleteGroundRoutes();

  public:
    RoleOracle();
    virtual ~RoleOracle();
    virtual simtime_t getUpdateFrequency() const override { return updateFrequency; };

    virtual void handleMessage(cMessage *msg) override;

};

} // namespace roleoracle

} // namespace inet

#endif // ifndef __INET_ROLEORACLE_H

