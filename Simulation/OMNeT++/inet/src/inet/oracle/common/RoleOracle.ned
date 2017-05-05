package inet.oracle.common;

import inet.oracle.contract.IRoleOracle;

module RoleOracle like IRoleOracle
{
    parameters:
        double updateFrequency @unit(s) = default(1s);
        
        bool overloadMaster = default(false);
        int targetMaster = default(1);
        
        bool useEnergies = default(false);
        double energyRankWeight = default(1.0);
        
        @display("i=misc/town_s");
        @class(RoleOracle);
}
