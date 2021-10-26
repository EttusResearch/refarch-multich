//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/*******************************************************************************************************************
Channel to Channel Loopback to Disk
single TX -> ALL RX.
If the user sets the number of samples to zero, this function will stream
continuously. WARNING: This can quickly create large files. The multithreaded version
currently has each USRP in its own thread. Receiving and Writing are not yet
seperate. This version uses one RX streamer per device.
*******************************************************************************************************************/

#include "blocksettings.hpp"
#include "graphassembly.hpp"
#include "receivefunctions.hpp"
#include "recvdata.hpp"
#include "replaycontrol.hpp"
#include "structures.hpp"
#include "sync.hpp"
#include <uhd/exception.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <thread>

volatile bool stop_signal_called = false;

typedef std::function<uhd::sensor_value_t(const std::string&)> get_sensor_fn_t;

/***********************************************************************
 * Loopback Function
 *
 **********************************************************************/
int singleTXLoopbackMultithread(GraphSettings& graphSettings,
    SignalSettings& signalSettings,
    DeviceSettings& deviceSettings)
{
    std::vector<size_t> rx_channel_nums;
    // thread group for multithreading
    boost::thread_group thread_group;

    rx_channel_nums.push_back(0);
    rx_channel_nums.push_back(1);


    uhd::time_spec_t now =
        graphSettings.graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now();
    graphSettings.time_spec = uhd::time_spec_t(now + signalSettings.rtime);
    int threadnum           = 0;
    // Receive graphSettings.rx_stream_vector.size()
    std::signal(SIGINT, &ReplayControl::sig_int_handler);

    if (signalSettings.format == "sc16") {
        for (int i = 0; i < graphSettings.rx_stream_vector.size(); i = i + 2) {
            std::cout << "Spawning RX Thread.." << threadnum << std::endl;
            thread_group.create_thread(std::bind(&ReceiveControl::recvToFileMultithread,
                graphSettings.rx_stream_vector[i],
                signalSettings.format,
                signalSettings.otw,
                signalSettings.rx_file,
                signalSettings.spb,
                signalSettings.nsamps,
                graphSettings.time_spec,
                rx_channel_nums,
                signalSettings.rx_timeout,
                deviceSettings.rx_rate,
                signalSettings.singleTX,
                signalSettings,
                0,
                deviceSettings,
                graphSettings,
                signalSettings.time_requested,
                threadnum));
            threadnum++;
        }

    } else {
        throw std::runtime_error("Unknown type " + signalSettings.format);
    }


    std::cout << "Replaying data (Press Ctrl+C to stop)..." << std::endl;

    // Todo: This is the same If and else
    if (signalSettings.nsamps <= 0) {
        // replay the entire buffer over and over
        std::cout << "Issuing replay command for " << signalSettings.samples_to_replay
                  << " samps in continuous mode..." << std::endl;


        graphSettings.replay_ctrls[signalSettings.singleTX]->config_play(
            graphSettings.replay_buff_addr,
            graphSettings.replay_buff_size,
            graphSettings.replay_chan_vector[signalSettings.singleTX]);

        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.num_samps  = signalSettings.nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec  = graphSettings.time_spec;
        graphSettings.replay_ctrls[signalSettings.singleTX]->issue_stream_cmd(
            stream_cmd, graphSettings.replay_chan_vector[signalSettings.singleTX]);

    } else {
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id()
                  << " Port: "
                  << graphSettings.replay_chan_vector[signalSettings.singleTX]
                  << std::endl;
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id()
                  << " Issuing replay command for " << signalSettings.nsamps
                  << " samps..." << std::endl;

        // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
        graphSettings.replay_ctrls[signalSettings.singleTX]->config_play(
            graphSettings.replay_buff_addr,
            graphSettings.replay_buff_size,
            graphSettings.replay_chan_vector[signalSettings.singleTX]);

        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        stream_cmd.num_samps  = signalSettings.nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec  = graphSettings.time_spec;
        graphSettings.replay_ctrls[signalSettings.singleTX]->issue_stream_cmd(
            stream_cmd, graphSettings.replay_chan_vector[signalSettings.singleTX]);
    }

    thread_group.join_all();

    // Remove SIGINT handler
    std::signal(SIGINT, SIG_DFL);
    return EXIT_SUCCESS;
}


/***********************************************************************
 * Main function
 **********************************************************************/

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // find configuration file -cfgFile adds to "desc" variable
    ProgramMetaData pmd;
    DeviceSettings device;
    SignalSettings signal;
    GraphSettings graphStruct;

    // INFO: Comment what each initilization does what type of data is stored in each.
    device.addProgramOptions(pmd.desc);
    signal.addProgramOptions(pmd.desc);

    pmd.storeProgramOptions(argc, argv);
    device.addAddresstoArgs();

    // determine folder name to store data in
    boost::posix_time::ptime timeLocal = boost::posix_time::second_clock::local_time();

    std::string month   = std::to_string(timeLocal.date().month());
    std::string day     = std::to_string(timeLocal.date().day());
    std::string year    = std::to_string(timeLocal.date().year());
    std::string hour    = std::to_string(timeLocal.time_of_day().hours());
    std::string minute  = std::to_string(timeLocal.time_of_day().minutes());
    std::string seconds = std::to_string(timeLocal.time_of_day().seconds());

    device.folder_name =
        month + day + year + "_" + hour + minute + seconds + "_" + signal.rx_file;
    
    for (int i = 0; i <32 ; i++){
    std::string file_location = ReceiveFunctions::generateRxFilename(signal.rx_file, i, 0, 0,
    device.tx_freq, device.folder_name, signal.rx_file_streamers,
    signal.rx_file_location);
        std::cout << file_location << std::endl;
    }
    
    return EXIT_SUCCESS;
}