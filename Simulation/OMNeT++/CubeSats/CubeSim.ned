import inet.common.figures.DelegateSignalConfigurator;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;

import inet.environment.common.PhysicalEnvironment;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;

import inet.linklayer.cubemac.CubeMacLayer;

import inet.node.aodv.AODVRouter;
import inet.node.inet.WirelessHost;

network Base
{
    parameters:
        int numMasters = default(1);
        int numSlaves = default(1);
        string radioMediumType = default("IdealRadioMedium");

		// Sandbox
        //@display("bgb=1000,1000;bgg=100,1,grey95");
        
        // Title
        @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);

        // Signal config and display
		@signal[dest_rcvdPk];
        @delegatesignal[rcvdPk](source=nodeMaster[0].udpApp[0].rcvdPk; target=dest_rcvdPk);
        @figure[rcvdPkText](type=indicatorText; pos=420,20; anchor=w; font=,20; textFormat="Packets received: %g"; initialValue=0);
        @statistic[rcvdPk](source=dest_rcvdPk; record=figure(count); targetFigure=rcvdPkText);

    submodules:
        visualizer: IntegratedCanvasVisualizer {
            @display("p=100,950");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=250,950");
        }
        radioMedium: <radioMediumType> like IRadioMedium {
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
        nodeMaster[numMasters]: AODVRouter{
            parameters:
                @display("is=s;r=,,#7F7F7F");
        }
        // Slaves
        nodeSlave[numSlaves]: AODVRouter{
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
        nodeGround[numGround]: AODVRouter{
            parameters:
                @display("is=s;r=,,#606060"); 
        }    
}
