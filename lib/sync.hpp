#ifndef SYNC_H
#define SYNC_H
#include "structures.hpp"

class SyncDevices
{
public:
    static void setSources(GraphSettings& graphSettings, const std::string& ref);
    static int syncAllDevices(GraphSettings& graphSettings);
    static void killLOs(GraphSettings& graphSettings, const std::vector<std::string>& lo);
    static void setLOsfromConfig(
        GraphSettings& graphSettings, const std::vector<std::string>& lo);
    static void setSource(int device, GraphSettings& graphSettings);
    static void setTerminal(int device, GraphSettings& graphSettings);
    static void setDistributor(int device, GraphSettings& graphSettings);
    static void checkRXSensorLock(GraphSettings& graphSettings);
    static void checkTXSensorLock(GraphSettings& graphSettings);
};

#endif