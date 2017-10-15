package inet.oracle.common;

import inet.oracle.contract.IRoleOracle;

module RoleOracle like IRoleOracle
{
    parameters:
        double updateFrequency @unit(s) = default(1s);
        
        int hysteresis = default(10);
        
        bool useEnergies = default(false);
        double energyRankWeight = default(1.0);
        
        double groundMasterCooldown = default(200.0);
        bool enforceSouthOfGround = default(false);
        
        bool useFixedTimes = default(false);
        bool useEnergyBudgets = default(false);
        
        double maxGroundMasterDuration = default(54);
        double energyBudget = default(-0.0001); // No idea what a good default is for this
        
        @display("i=misc/town_s");
        @class(RoleOracle);
        
        @signal[groundMasterChanged](type=long);
        @statistic[groundMaster](title="groundMaster"; source=groundMasterChanged; record=count,vector);
}
