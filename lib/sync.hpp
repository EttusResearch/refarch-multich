#ifndef SYNC_H 
#define SYNC_H
#include "structures.hpp"

class SyncDevices
{
    public:
        static void setSources(ProgramMetaData& pmd, DeviceSettings& device, GraphSettings& graphSettings);  
        static int syncAllDevices(GraphSettings& graphSettings);
        static void killLOs(GraphSettings& graphSettings, DeviceSettings& deviceSettings);
        static void setLOsfromConfig(GraphSettings& graphSettings, DeviceSettings& deviceSettings);
        static void setSource(int device, GraphSettings& graphSettings);
        static void setTerminal(int device, GraphSettings& graphSettings);
        static void setDistributor(int device, GraphSettings& graphSettings);
        static void checkRXSensorLock(GraphSettings& graphSettings);
        static void checkTXSensorLock(GraphSettings& graphSettings);
};

#endif