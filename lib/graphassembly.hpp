#ifndef GRAPHASSEMBLY_H
#define GRAPHASSEMBLY_H
#include "structures.hpp"


class GraphAssembly
{
    
    public:
        //methods
        static void buildGraph(GraphSettings& graphSettings, const DeviceSettings& deviceSettings);
        static void buildRadios(GraphSettings& graphSettings);
        static void buildDDCDUC(GraphSettings& graphSettings);
        static void buildReplay(GraphSettings& graphSettings); 
        static void buildStreams(GraphSettings& graphSettings, DeviceSettings& deviceSettings, SignalSettings& signalSettings); 
        static void connectGraph(GraphSettings& graphSettings, SignalSettings& signalSettings);
        static void commitGraph(GraphSettings& graphSettings);
        static void connectGraphMultithread(GraphSettings& graphSettings);
        static void buildStreamsMultithread(GraphSettings& graphSettings, DeviceSettings& deviceSettings, SignalSettings& signalSettings);
};

#endif