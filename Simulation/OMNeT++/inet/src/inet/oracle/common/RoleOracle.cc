#include "inet/common/ModuleAccess.h"

#include "inet/oracle/common/RoleOracle.h"

namespace inet {

namespace roleoracle {

Define_Module(RoleOracle);

RoleOracle::RoleOracle() :
    updateFrequency(NaN)
{
}

RoleOracle::~RoleOracle()
{

}

void RoleOracle::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
    {
        updateFrequency = par("updateFrequency");
    }
}

} // namespace roleoracle

} // namespace inet

