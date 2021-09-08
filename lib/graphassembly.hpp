#ifndef GRAPHASSEMBLY_H
#define GRAPHASSEMBLY_H


class GraphAssembly
{
    
    public:
        //methods
        static void buildGraph(GraphSettings& graphSettings, const DeviceSettings& deviceSettings);
        static void buildRadios(GraphSettings& graphSettings, DeviceSettings& deviceSettings);
        static void buildDDCDUC(GraphSettings& graphSettings, DeviceSettings& deviceSettings);
        static void buildReplay(GraphSettings& graphSettings); 
        static void buildStreams(GraphSettings& graphSettings, DeviceSettings& deviceSettings, SignalSettings& signalSettings); 
        static void connectGraph(GraphSettings& graphSettings, SignalSettings& signalSettings);
        static void commitGraph(GraphSettings& graphSettings);
        static void buildStreamsMultithread(GraphSettings& graphSettings, DeviceSettings& deviceSettings, SignalSettings& signalSettings);
        static void connectGraphMultithread(GraphSettings& graphSettings, SignalSettings& signalSettings);
        static void connectGraphMultithread16(GraphSettings& graphSettings, SignalSettings& signalSettings);
        static void buildStreamsMultithread16(GraphSettings& graphSettings, DeviceSettings& deviceSettings, SignalSettings& signalSettings);
};

#endif