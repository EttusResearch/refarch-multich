#ifndef REPLAYCONTROL_H
#define REPLAYCONTROL_H

class ReplayControl{

    public:
        static int importData(GraphSettings& graphSettings, SignalSettings& signalSettings);
        static int runTXRXiterativeloopback(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings);
        static void stopReplay(GraphSettings& graphSettings);
        static int singleTXLoopback(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings);
        static void sig_int_handler(int);
        static int singleTXLoopbackMultithread(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings);
        static int allTX(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings);
        static int allRX(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings);
        



};

#endif