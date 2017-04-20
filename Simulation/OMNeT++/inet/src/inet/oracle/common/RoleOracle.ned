package inet.oracle.common;

import inet.oracle.contract.IRoleOracle;

module RoleOracle like IRoleOracle
{
    parameters:
        int updateFrequency = default(1); // module path of the geographic coordinate system module
        @display("i=misc/town_s");
        @class(RoleOracle);
}
