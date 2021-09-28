//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#ifndef RECVTOFILE_H
#define RECVTOFILE_H

#include "structures.hpp"
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
class ReceiveControl
{
public:
    static void recvToFile(uhd::rx_streamer::sptr rx_stream,
        const std::string& cpu_format,
        const std::string& wire_format,
        const std::string& file,
        size_t samps_per_buff,
        int num_requested_samples,
        uhd::time_spec_t start_time,
        std::vector<size_t> rx_channel_nums,
        double timeout,
        int rx_rate,
        int tx_chan_num,
        SignalSettings& signalSettings,
        int numrepeat,
        DeviceSettings& deviceSettings,
        GraphSettings& graphSettings,
        double time_requested);

    static void recvToMemMultithread(uhd::rx_streamer::sptr rx_stream,
        const std::string& cpu_format,
        const std::string& wire_format,
        const std::string& file,
        size_t samps_per_buff,
        int num_requested_samples,
        uhd::time_spec_t start_time,
        std::vector<size_t> rx_channel_nums,
        double timeout,
        int rx_rate,
        int tx_chan_num,
        SignalSettings& signalSettings,
        int numrepeat,
        DeviceSettings& deviceSettings,
        GraphSettings& graphSettings,
        double time_requested,
        int threadnum);

    static void recvToFileMultithread(uhd::rx_streamer::sptr rx_stream,
        const std::string& cpu_format,
        const std::string& wire_format,
        const std::string& file,
        size_t samps_per_buff,
        int num_requested_samples,
        uhd::time_spec_t start_time,
        std::vector<size_t> rx_channel_nums,
        double timeout,
        int rx_rate,
        int tx_chan_num,
        SignalSettings& signalSettings,
        int numrepeat,
        DeviceSettings& deviceSettings,
        GraphSettings& graphSettings,
        double time_requested,
        int threadnum);
};


#endif