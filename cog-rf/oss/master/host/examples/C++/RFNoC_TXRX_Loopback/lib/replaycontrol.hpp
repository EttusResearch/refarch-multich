#ifndef REPLAYCONTROL_H
#define REPLAYCONTROL_H

class ReplayControl{

    public:
        static int importData(GraphSettings& graphSettings, SignalSettings& signalSettings);
        static int runTXRXiterativeloopback(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings);
        static void stopReplay(GraphSettings& graphSettings);
        static void replayTransmit(GraphSettings& graphSettings, SignalSettings& signalSettings);
        static void rfnocReceive(GraphSettings& graphSettings, double startTime);
        static int singleTXLoopback(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings);
        static int multiCWloopback(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd);
        static void runSelector( GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd );
        static void sig_int_handler(int);
        static int singleTXLoopbackMultithread(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings);
        //static int multithreadLoopback(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings);

};

#endif