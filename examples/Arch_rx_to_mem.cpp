//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/*******************************************************************************************************************
ALL RX to Memory.
If the user sets the number of samples to zero, this function will stream
continuously. The multithreaded version
currently has each USRP in its own thread. This version uses one RX streamer per device.
*******************************************************************************************************************/

#include "RefArch.hpp"
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/utils/safe_main.hpp>
#include <csignal>
#include <thread>

class rx_only : public RefArch
{
public:
    using RefArch::RefArch;
    void spawnReceiveThreads() override
    {
        int threadnum = 0;

        std::signal(SIGINT, this->sigIntHandler);
        std::vector<std::thread> vectorThread;
        // Receive RA_rx_stream_vector.size()
        if (RA_format == "sc16") {
            for (int i = 0; i < RA_rx_stream_vector.size(); i = i + 2) {
                std::cout << "Spawning RX Thread.." << threadnum << std::endl;
                std::thread t(
                    [this](int threadnum, uhd::rx_streamer::sptr rx_streamer) {
                        recv(2, threadnum, rx_streamer);
                    },
                    threadnum,
                    RA_rx_stream_vector[i]);

                vectorThread.push_back(std::move(t));
                threadnum++;
            }
        } else {
            throw std::runtime_error("Unknown type " + RA_format);
        }
        std::cout << "Receiving data (Press Ctrl+C to stop)..." << std::endl;

        for (auto& i : vectorThread) {
            i.join();
        }
        std::signal(SIGINT, SIG_DFL);

        return;
    }
    void connectGraphMultithread() override
    {
        // This is the function that connects the graph for the multithreaded
        // implementation The difference is that each device gets its own RX streamer.
        UHD_LOG_INFO("CogRF", "Connecting graph...");
        std::cout << RA_rx_stream_vector.size();
        std::cout << RA_radio_ctrls.size();
        // Connect Graph
        for (int i = 0; i < RA_radio_ctrls.size(); i++) {
            // connect radios to ddc
            RA_graph->connect(
                RA_radio_block_list[i], 0, RA_ddc_ctrls[i]->get_block_id(), 0);
            std::cout << "Connected " << RA_radio_block_list[i] << " to "
                      << RA_ddc_ctrls[i]->get_block_id() << std::endl;
        }

        for (int j = 0; j < RA_ddc_ctrls.size(); j++) {
            // Connect DDC to streamers
            // Reusing replay chan vector, need a vector of zeros and ones
            RA_graph->connect(RA_ddc_ctrls[j]->get_block_id(),
                0,
                RA_rx_stream_vector[j],
                div(j, 2).rem);

            std::cout << "Connected " << RA_ddc_ctrls[j]->get_block_id() << " to "
                      << RA_rx_stream_vector[j] << " Port " << div(j, 2).rem << std::endl;
        }
    }
};

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // find configuration file -cfgFile adds to "desc" variable
    rx_only usrpSystem(argc, argv);
    usrpSystem.parseConfig();
    // Setup Graph with input Arguments
    usrpSystem.buildGraph();
    // Sync Device Clocks
    usrpSystem.setSources();
    // Setup Radio Blocks
    usrpSystem.buildRadios();
    // Setup DDC/DUC Blocks
    usrpSystem.buildDDCDUC();
    // Setup Replay Blocks
    // usrpSystem.buildReplay();
    // Setup LO distribution
    usrpSystem.setLOsfromConfig();
    // Set Radio Block Settings
    usrpSystem.setRadioRates();
    // Tune RX
    usrpSystem.tuneRX();
    // Tune TX
    // usrpSystem.tuneTX();
    // set RX Gain
    usrpSystem.setRXGain();
    // set TX Gain
    // usrpSystem.setTXGain();
    // set RX bandwidth
    usrpSystem.setRXBw();
    // set TX bandwidth
    // usrpSystem.setTXBw();
    // set RX Antenna
    usrpSystem.setRXAnt();
    // set TX Antenna
    // usrpSystem.setTXAnt();
    // Check RX Sensor Lock
    usrpSystem.checkRXSensorLock();
    // Check TX Sensor Lock
    // usrpSystem.checkTXSensorLock();
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
    // Sync times across threads
    usrpSystem.updateDelayedStartTime();
    usrpSystem.spawnReceiveThreads();
    std::cout << "Run complete." << std::endl;
    // Kill Replay
    usrpSystem.stopReplay();
    // Kill LO
    usrpSystem.killLOs();

    std::cout << std::endl << "Closing USRP Sessions" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
