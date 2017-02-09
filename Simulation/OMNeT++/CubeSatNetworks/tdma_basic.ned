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
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.INetworkNode;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.linklayer.lmac.LMacLayer;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import inet.environment.common.PhysicalEnvironment;

network Base
{
    parameters:
        string hostType = default("WirelessHost");
        string mediumType = default("IdealRadioMedium");

        @display("bgb=650,500;bgg=100,1,grey95");
        @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);

        @figure[rcvdPkText](type=indicatorText; pos=420,20; anchor=w; font=,20; textFormat="packets received: %g"; initialValue=0);
        @statistic[rcvdPk](source=hostM1_rcvdPk; record=figure(count); targetFigure=rcvdPkText);
        @signal[hostM1_rcvdPk];
        @delegatesignal[rcvdPk](source=hostM1.udpApp[0].rcvdPk; target=hostM1_rcvdPk);

    submodules: // Need to define loops for node creation
        visualizer: IntegratedCanvasVisualizer {
            @display("p=580,125");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=580,200");
        }
        radioMedium: <mediumType> like IRadioMedium {
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
        hostM0: <hostType> like INetworkNode {
            @display("p=50,325");
        }
        hostM1: <hostType> like INetworkNode {
            @display("p=450,325");
        }
        // Slaves
        hostS00: <hostType> like INetworkNode {
            @display("p=250,300");
        }
        hostS01: <hostType> like INetworkNode {
            @display("p=150,450");
        }
        hostS02: <hostType> like INetworkNode {
            @display("p=350,450");
        }
}
