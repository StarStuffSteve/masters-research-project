package inet.oracle.common;

import inet.oracle.contract.IRoleOracle;

module RoleOracle like IRoleOracle
{
    parameters:
        double updateFrequency @unit(s) = default(1s);
        @display("i=misc/town_s");
        @class(RoleOracle);
}
