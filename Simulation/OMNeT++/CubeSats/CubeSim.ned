import inet.common.figures.DelegateSignalConfigurator;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;

import inet.environment.common.PhysicalEnvironment;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;

import inet.node.inet.INetworkNode;
import inet.node.aodv.AODVRouter;
import inet.node.dymo.DYMORouter;
import inet.node.inet.WirelessHost;

import inet.linklayer.cubemac.CubeMacLayer;

import inet.oracle.common.RoleOracle;

network Base
{
    parameters:
        int numMasters = default(1);
        int numSlaves = default(1);
        string radioMediumType = default("IdealRadioMedium");
        string hostType = default("DYMORouter");

		// Sandbox
        @display("bgb=1000,1000;bgg=100,1,grey95");
        
        // Title
        @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);

        // Signal config and display
		@signal[dest_rcvdPk];
        @delegatesignal[rcvdPk](source=nodeMaster[0].udpApp[0].rcvdPk; target=dest_rcvdPk);
        @figure[rcvdPkText](type=indicatorText; pos=420,20; anchor=w; font=,20; textFormat="Packets received: %g"; initialValue=0);
        @statistic[rcvdPk](source=dest_rcvdPk; record=figure(count); targetFigure=rcvdPkText);

    submodules:
        roleOracle: RoleOracle {
            @display("p=100,850");
        }
        
        visualizer: IntegratedCanvasVisualizer {
            @display("p=100,950");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=250,950");
        }
        radioMedium[2]: <radioMediumType> like IRadioMedium {
            @display("p=400,950");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=550,950");
        }
        figureHelper: DelegateSignalConfigurator {
            @display("p=700,950");
        }
        
        //
        // --- Nodes
        //
        // Masters
        nodeMaster[numMasters]: <hostType> like INetworkNode {
            parameters:
                @display("is=s;r=,,#7F7F7F");
        }
        // Slaves
        nodeSlave[numSlaves]: <hostType> like INetworkNode {
            parameters:
                @display("is=s;r=,,#606060");
        }
}

network GroundStation extends Base
{
    parameters:
        int numGround = default(1);
        
        // Signal config and display
		@signal[dest_rcvdPk];
        @delegatesignal[rcvdPk](source=nodeGround[0].udpApp[0].rcvdPk; target=dest_rcvdPk);
        @figure[rcvdPkText](type=indicatorText; pos=420,20; anchor=w; font=,20; textFormat="Packets received: %g"; initialValue=0);
        @statistic[rcvdPk](source=dest_rcvdPk; record=figure(count); targetFigure=rcvdPkText);
        
    submodules:
        // Ground Station
        nodeGround[numGround]: <hostType> like INetworkNode {
            parameters:
                @display("is=s;r=,,#606060"); 
        }    
}
