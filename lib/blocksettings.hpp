//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef BLOCKSETTINGS_H
#define BLOCKSETTINGS_H
#include "structures.hpp"

class BlockSettings
{
public:
    static int setRadioRates(
        GraphSettings& graphSettings, double& rx_rate, double& tx_rate);
    static void tuneRX(GraphSettings& graphSettings, const double& rx_freq);
    static void tuneTX(GraphSettings& graphSettings, const double& tx_freq);
    static void setRXGain(GraphSettings& graphSettings, const double& rx_gain);
    static void setTXGain(GraphSettings& graphSettings, const double& tx_gain);
    static void setRXBw(GraphSettings& graphSettings, const double& rx_bw);
    static void setTXBw(GraphSettings& graphSettings, const double& tx_bw);
    static void setRXAnt(GraphSettings& graphSettings, const std::string& rx_ant);
    static void setTXAnt(GraphSettings& graphSettings, const std::string& tx_ant);
};


#endif