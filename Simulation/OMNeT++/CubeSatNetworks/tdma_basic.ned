//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2003 Ahmet Sekercioglu
// Copyright (C) 2003-2015 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

import inet.common.figures.DelegateSignalConfigurator;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;

import inet.environment.common.PhysicalEnvironment;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.linklayer.lmac.LMacLayer;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.WirelessHost;
import inet.node.aodv.AODVRouter;

network Base
{
    parameters:
        int numMasters;
//        int numSlaves;
        string radioMediumType = default("IdealRadioMedium");

        @display("bgb=650,500;bgg=100,1,grey95");
        @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);

		@signal[nodeMaster0_rcvdPk];
        @figure[rcvdPkText](type=indicatorText; pos=420,20; anchor=w; font=,20; textFormat="packets received: %g"; initialValue=0);
        @statistic[rcvdPk](source=nodeMaster0_rcvdPk; record=figure(count); targetFigure=rcvdPkText);
        @delegatesignal[rcvdPk](source=nodeMaster[0].udpApp[0].rcvdPk; target=nodeMaster0_rcvdPk);

    submodules: // Need to define loops for node creation
        visualizer: IntegratedCanvasVisualizer {
            @display("p=580,125");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=580,200");
        }
        radioMedium: <radioMediumType> like IRadioMedium {
            @display("p=580,275");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=580,425");
        }
        figureHelper: DelegateSignalConfigurator {
            @display("p=580,350");
        }
        //
        // --- Nodes
        //
        // Masters
        nodeMaster[numMasters]: AODVRouter{
            parameters:
                @display("r=,,#707070");
        }
        // Slaves
//        nodeSlave[numSlaves]: WirelessHost{
//            parameters:
//                @display("r=,,#707070"); 
//        }    
}
