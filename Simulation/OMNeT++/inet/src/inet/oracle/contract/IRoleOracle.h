#ifndef __INET_IROLEORACLE_H
#define __INET_IROLEORACLE_H

// Removing all includes causes a syntx error for some reason
#include "inet/common/geometry/common/Coord.h"

namespace inet {

namespace roleoracle {

class INET_API IRoleOracle
{
  public:
    virtual int getUpdateFrequency() const = 0;
};

} // namespace roleoracle

} // namespace inet

#endif // ifndef __INET_IROLEORACLE_H

