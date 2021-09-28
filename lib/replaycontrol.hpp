#ifndef REPLAYCONTROL_H
#define REPLAYCONTROL_H
#include "structures.hpp"

class ReplayControl
{
public:
    static int importData(
        GraphSettings& graphSettings, const std::string& file, size_t& samples_to_replay);;
    static void stopReplay(GraphSettings& graphSettings);
    static void sig_int_handler(int);
};

#endif