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

#include "RefArch.hpp"
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <stdio.h>
#include <boost/circular_buffer.hpp>
#include <csignal>
#include <fstream>
#include <memory>
#include <thread>
#include <csignal>

class iterative_loopback : public RefArch
{
    using RefArch::RefArch;

public:
    std::string folder_name;
    size_t run_number;

    std::string zeropad_to_length(int length, std::string s)
    {
        return std::string(length - s.length(), '0') + s;
    }

    void localTime()
    {
        // clang-format off
        boost::posix_time::ptime timeLocal = boost::posix_time::second_clock::local_time();
        std::string month = zeropad_to_length(2, std::to_string(timeLocal.date().month()));
        std::string day  = zeropad_to_length(2, std::to_string(timeLocal.date().day()));
        std::string year = zeropad_to_length(4, std::to_string(timeLocal.date().year()));
        std::string hour = zeropad_to_length(2, std::to_string(timeLocal.time_of_day().hours()));
        std::string minute = zeropad_to_length(2, std::to_string(timeLocal.time_of_day().minutes()));
        std::string seconds = zeropad_to_length(2, std::to_string(timeLocal.time_of_day().seconds()));
        folder_name = month + day + year + "_" + hour + minute + seconds + "_" + RA_rx_file;
        // clang-format on
    }

    void recv(
        int rx_channel_nums, int threadnum, uhd::rx_streamer::sptr rx_streamer) override
    {
        uhd::set_thread_priority_safe(0.9F);
        int num_total_samps = 0;
        std::unique_ptr<char[]> buf(new char[RA_spb]);
        // Prepare buffers for received samples and metadata
        uhd::rx_metadata_t md;
        std::vector<boost::circular_buffer<std::complex<short>>> buffs(
            rx_channel_nums, boost::circular_buffer<std::complex<short>>(RA_spb + 1));
        // create a vector of pointers to point to each of the channel buffers
        std::vector<std::complex<short>*> buff_ptrs;
        for (size_t i = 0; i < buffs.size(); i++) {
            buff_ptrs.push_back(&buffs[i].front());
        }
        // Correctly lable output files based on run method, single TX->single RX or
        // single TX
        // -> All RX
        int rx_identifier = threadnum;
        std::vector<std::shared_ptr<std::ofstream>> outfiles;
        for (size_t i = 0; i < buffs.size(); i++) {
            // rx_identifier * 2 + i in order to get correct channel number in filename
            const std::string this_filename = generateRxFilename(RA_file,
                rx_identifier * 2 + i,
                RA_singleTX,
                run_number, // Cleanup: The only thing different between this function and
                            // to-file.
                RA_tx_freq,
                folder_name,
                RA_rx_file_channels,
                RA_rx_file_location);
            auto outstream = std::shared_ptr<std::ofstream>(new std::ofstream());
            outstream->rdbuf()->pubsetbuf(buf.get(), RA_spb); // Important
            outstream->open(this_filename.c_str(), std::ofstream::binary);
            outfiles.push_back(std::shared_ptr<std::ofstream>(outstream));
        }
        UHD_ASSERT_THROW(outfiles.size() == buffs.size());
        UHD_ASSERT_THROW(buffs.size() == rx_channel_nums);
        bool overflow_message = true;
        // setup streaming
        uhd::stream_cmd_t stream_cmd(
            (RA_nsamps == 0) ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                             : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        stream_cmd.num_samps  = RA_nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec  = RA_start_time;
        md.has_time_spec      = true;
        md.time_spec          = RA_start_time;
        const auto stop_time  = RA_start_time + uhd::time_spec_t(RA_time_requested);
        rx_streamer->issue_stream_cmd(stream_cmd);
        int loop_num = 0;
        while (not RA_stop_signal_called
               and (RA_nsamps > num_total_samps or RA_nsamps == 0)
               and (RA_time_requested == 0.0
                    or RA_graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now()
                           <= stop_time)) {
            size_t num_rx_samps = rx_streamer->recv(buff_ptrs, RA_spb, md, RA_rx_timeout);
            loop_num += 1;
            if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
                std::cout << boost::format("Timeout while streaming") << std::endl;
                break;
            }
            if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
                if (overflow_message) {
                    overflow_message    = false;
                    std::string tempstr = "\n thread:" + std::to_string(threadnum) + '\n'
                                          + "loop_num:" + std::to_string(loop_num) + '\n';
                    std::cout << tempstr;
                }
                if (md.out_of_sequence != true) {
                    std::cerr
                        << boost::format(
                               "Got an overflow indication. Please consider the "
                               "following:\n"
                               "  Your write medium must sustain a rate of %fMB/s.\n"
                               "  Dropped samples will not be written to the file.\n"
                               "  Please modify this example for your purposes.\n"
                               "  This message will not appear again.\n")
                               % (RA_rx_rate * sizeof(std::complex<short>) / 1e6);
                    break;
                }
                continue;
            }
            if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
                throw std::runtime_error(
                    str(boost::format("Receiver error %s") % md.strerror()));
            }
            num_total_samps += num_rx_samps * rx_streamer->get_num_channels();
            for (size_t i = 0; i < outfiles.size(); i++) {
                outfiles[i]->write((const char*)buff_ptrs[i],
                    num_rx_samps * sizeof(std::complex<short>));
            }
        }
        // Shut down receiver
        stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
        rx_streamer->issue_stream_cmd(stream_cmd);

        for (size_t i = 0; i < outfiles.size(); i++) {
            outfiles[i]->close();
        }
        std::cout << "Thread: " << threadnum << " Received: " << num_total_samps
                  << " samples..." << std::endl;
    }
};
/***********************************************************************
 * Main function
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // find configuration file -cfgFile adds to "desc" variable
    iterative_loopback usrpSystem(argc, argv);

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
    usrpSystem.localTime();
    std::signal(SIGINT, usrpSystem.sigIntHandler);
    int saved_user_delay_time = usrpSystem.RA_delay_start_time;
    for (usrpSystem.run_number = 0; usrpSystem.run_number < usrpSystem.RA_nruns;
         usrpSystem.run_number++) {
        for (usrpSystem.RA_singleTX = 0;
             usrpSystem.RA_singleTX < usrpSystem.RA_replay_ctrls.size();
             usrpSystem.RA_singleTX++) {
            usrpSystem.RA_stop_signal_called = false;
            // Calculate starttime for threads
            usrpSystem.updateDelayedStartTime();
            usrpSystem.spawnReceiveThreads();
            usrpSystem.transmitFromReplay();
            usrpSystem.joinAllThreads();
            // Next iteration use saved_user_delay_time
            usrpSystem.RA_delay_start_time = saved_user_delay_time;
        }
        // Next iteration use RA_rep_delay
        usrpSystem.RA_delay_start_time = usrpSystem.RA_rep_delay;
    }
    //Join Threads
    std::signal(SIGINT, SIG_DFL);
    std::cout << "Run complete." << std::endl;
    // Kill Replay
    usrpSystem.stopReplay();
    // Kill LO
    usrpSystem.killLOs();
    std::cout << std::endl << "Closing USRP Sessions" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
