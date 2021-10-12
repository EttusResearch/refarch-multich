//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

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