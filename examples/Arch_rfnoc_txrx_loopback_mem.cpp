//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/*******************************************************************************************************************
Channel to Channel Loopback to Memory
single TX -> ALL RX.
If the user sets the number of samples to zero, this function will stream
continuously. The multithreaded version
currently has each USRP in its own thread. This version uses one RX streamer per device.
*******************************************************************************************************************/

#include "ArchUSRP.hpp"
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/utils/safe_main.hpp>
#include <csignal>
#include <thread>

bool ArchUSRP::stop_signal_called=false;
/***********************************************************************
 * Loopback Function
 **********************************************************************/
class rfnoc_txrx_loopback_mem: public ArchUSRP{
    public:

    using ArchUSRP::ArchUSRP;

    void example()
    {
        uhd::time_spec_t now =
            graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now();
        time_spec = uhd::time_spec_t(now + rtime);
        int threadnum = 0;
        
        std::signal(SIGINT, this->sig_int_handler);
        std::vector<std::thread> vectorThread;
        // Receive rx_stream_vector.size()
        if (format == "sc16") {
            for (int i = 0; i < rx_stream_vector.size(); i = i + 2) {
                std::cout << "Spawning RX Thread.." << threadnum << std::endl;
                /*std::thread t (passthru,
                    this,
                    2,
                    threadnum,
                    rx_stream_vector[i]);*/
                std::thread t ( [this]
                    (int threadnum,uhd::rx_streamer::sptr rx_streamer)
                    {recv(2,threadnum,rx_streamer);}, 
                    threadnum,rx_stream_vector[i]);

                vectorThread.push_back(std::move(t));
                threadnum++;
            }
        } else {
            throw std::runtime_error("Unknown type " + format);
        }
        std::cout << "Replaying data (Press Ctrl+C to stop)..." << std::endl;
        // Todo: This is the same If and else
        if (nsamps <= 0) {
            // replay the entire buffer over and over
            std::cout << "Issuing replay command for " << samples_to_replay
                    << " samps in continuous mode..." << std::endl;

            replay_ctrls[singleTX]->config_play(
                replay_buff_addr,
                replay_buff_size,
                replay_chan_vector[singleTX]);

            uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
            stream_cmd.num_samps  = nsamps;
            stream_cmd.stream_now = false;
            stream_cmd.time_spec  = time_spec;
            replay_ctrls[singleTX]->issue_stream_cmd(
                stream_cmd, replay_chan_vector[singleTX]);

        } else {
            std::cout << replay_ctrls[singleTX]->get_block_id()
                    << " Port: "
                    << replay_chan_vector[singleTX]
                    << std::endl;
            std::cout << replay_ctrls[singleTX]->get_block_id()
                    << " Issuing replay command for " << nsamps
                    << " samps..." << std::endl;

            // Replay nsamps, wrapping back to the start of the buffer if nsamps is
            // larger than the buffer size.
            replay_ctrls[singleTX]->config_play(
                replay_buff_addr,
                replay_buff_size,
                replay_chan_vector[singleTX]);

            uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
            stream_cmd.num_samps  = nsamps;
            stream_cmd.stream_now = false;
            stream_cmd.time_spec  = time_spec;
            replay_ctrls[singleTX]->issue_stream_cmd(
                stream_cmd, replay_chan_vector[singleTX]);
        }
        for (auto &i : vectorThread){
            i.join();
        }
        std::signal(SIGINT, SIG_DFL);

        return;
    }

};

/***********************************************************************
 * Main function
 **********************************************************************/

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // find configuration file -cfgFile adds to "desc" variable
    rfnoc_txrx_loopback_mem usrpSystem(argc, argv);
    
    // Setup Graph with input Arguments
    usrpSystem.buildGraph();
    // Sync Device Clocks
    usrpSystem.setSources();
    // Setup Radio Blocks
    usrpSystem.buildRadios();
    // Setup DDC/DUC Blocks
    usrpSystem.buildDDCDUC();
    // Setup Replay Blocks
    usrpSystem.buildReplay();
    // Setup LO distribution
    usrpSystem.setLOsfromConfig();
    // Set Radio Block Settings
    usrpSystem.setRadioRates();
    // Tune RX
    usrpSystem.tuneRX();
    // Tune TX
    usrpSystem.tuneTX();
    // set RX Gain
    usrpSystem.setRXGain();
    // set TX Gain
    usrpSystem.setTXGain();
    // set RX bandwidth
    usrpSystem.setRXBw();
    // set TX bandwidth
    usrpSystem.setTXBw();
    // set RX Antenna
    usrpSystem.setRXAnt();
    // set TX Antenna
    usrpSystem.setTXAnt();
    // Check RX Sensor Lock
    usrpSystem.checkRXSensorLock();
    // Check TX Sensor Lock
    usrpSystem.checkTXSensorLock();
    // Build Streams
    usrpSystem.buildStreamsMultithread();
    // Connect Graph
    usrpSystem.connectGraphMultithread();
    // Commit Graph
    usrpSystem.commitGraph();
    // Allow for some setup time
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // Load Replay Block Buffers with data to transmit
    usrpSystem.importData();
    // Sync time across devices
    usrpSystem.syncAllDevices();
    // Begin TX and RX
    // INFO: Comment what each initilization does what type of data is stored in each.
    usrpSystem.example();
    std::cout << "Run complete." << std::endl;
    // Kill Replay
    usrpSystem.stopReplay();
    // Kill LO
    usrpSystem.killLOs();

    std::cout << std::endl << "Closing USRP Sessions" << std::endl << std::endl;
    return EXIT_SUCCESS;
}