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
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;

import inet.linklayer.cubemac.CubeMacLayer;

import inet.node.aodv.AODVRouter;
import inet.node.inet.WirelessHost;

network Base
{
    parameters:
        int numMasters;
        int numSlaves;
        int slavesPerMaster; // Not in use
        string radioMediumType = default("IdealRadioMedium");

        @display("bgb=1000,1000;bgg=100,1,grey95");
        @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);

		@signal[nodeMaster0_rcvdPk];
        @figure[rcvdPkText](type=indicatorText; pos=420,20; anchor=w; font=,20; textFormat="packets received: %g"; initialValue=0);
        @statistic[rcvdPk](source=nodeMaster0_rcvdPk; record=figure(count); targetFigure=rcvdPkText);
        @delegatesignal[rcvdPk](source=nodeMaster[0].udpApp[0].rcvdPk; target=nodeMaster0_rcvdPk);

    submodules: // Need to define loops for node creation
        visualizer: IntegratedCanvasVisualizer {
            @display("p=100,25");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=100,75");
        }
        radioMedium: <radioMediumType> like IRadioMedium {
            @display("p=100,125");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=100,175");
        }
        figureHelper: DelegateSignalConfigurator {
            @display("p=100,225");
        }
        //
        // --- Nodes
        //
        // Masters
        nodeMaster[numMasters]: AODVRouter{
            parameters:
                @display("is=s;r=,,#707070");
        }
        // Slaves
        // Has to be AODV or else packets will never get picker up
        // by local cluster master for reasons yet unknown
        nodeSlave[numSlaves]: AODVRouter{
            parameters:
                @display("is=s;r=,,#606060"); 
        }    
}
