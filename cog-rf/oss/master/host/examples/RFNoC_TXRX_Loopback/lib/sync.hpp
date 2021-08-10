#ifndef SYNC_H 
#define SYNC_H

class SyncDevices
{
    public:
        static void setSources(ProgramMetaData& pmd, DeviceSettings& device, GraphSettings& graphSettings);  
        static void setLOs(GraphSettings& graphSettings, const ProgramMetaData& pmd, DeviceSettings& device);
        static int syncAllDevices(GraphSettings& graphSettings);
        static void killLOs(GraphSettings& graphSettings, DeviceSettings& deviceSettings);
        static void setLOsfromConfig(GraphSettings& graphSettings, const ProgramMetaData& pmd, DeviceSettings& deviceSettings);
        static void setSource(int device, GraphSettings& graphSettings);
        static void setTerminal(int device, GraphSettings& graphSettings);
        static void setDistributor(int device, GraphSettings& graphSettings);
        static void checkRXSensorLock(GraphSettings& graphSettings);
        static void checkTXSensorLock(GraphSettings& graphSettings);
};

#endif