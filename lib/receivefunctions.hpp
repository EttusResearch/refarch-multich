//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef RECEIVEFUNCTIONS_H
#define RECEIVEFUNCTIONS_H
#include "structures.hpp"

class ReceiveFunctions
{
public:
    static std::string generateOutFilename(const std::string& base_fn,
        const size_t& rx_chan_num,
        const int& tx_chan_num,
        const int& run_num,
        const double& tx_freq,
        const std::string& folder_name);
    static std::string generateOutFilenameMultithread(const std::string& base_fn,
        const size_t& rx_chan_num,
        const int& tx_chan_num,
        const int& run_num,
        const double& tx_freq,
        const std::string& folder_name,
        const int& threadnum);
};

#endif