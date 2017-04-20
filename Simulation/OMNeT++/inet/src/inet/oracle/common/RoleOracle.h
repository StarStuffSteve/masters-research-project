#ifndef __INET_ROLEORACLE_H
#define __INET_ROLEORACLE_H

#include "inet/common/geometry/common/CoordinateSystem.h"

#include "inet/oracle/contract/IRoleOracle.h"

namespace inet {

namespace roleoracle {

class INET_API RoleOracle : public cModule, public IRoleOracle
{
  protected:
    int updateFrequency;
    virtual void initialize(int stage) override;

  public:
    RoleOracle();
    virtual ~RoleOracle();
    virtual int getUpdateFrequency() const override { return updateFrequency; }

};

} // namespace roleoracle

} // namespace inet

#endif // ifndef __INET_ROLEORACLE_H

