//
// Copyright 2021-2022 Ettus Research, a National Instruments Brand
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

class Arch_multifreq_loopback : public RefArch
{
public:
    using RefArch::RefArch;
    using RefArch::tuneRX;
    using RefArch::tuneTX;
    std::string folder_name;

    std::string zeropad_to_length(int length, std::string s)
    {
        return std::string(length - s.length(), '0') + s;
    }
    void tuneRX(double rx_freq)
    {
        RA_rx_freq = rx_freq;
        tuneRX();
        checkRXSensorLock();
    }
    void tuneTX(double tx_freq)
    {
        RA_tx_freq = tx_freq;
        tuneTX();
        checkRXSensorLock();
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
        int rx_channel_nums, int threadnum, uhd::rx_streamer::sptr rx_streamer, bool bw_summary, bool stats) override
    {
        uhd::set_thread_priority_safe(0.9F);
        size_t num_total_samps = 0;
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
        // Correctly label output files based on run method, single TX->single RX or
        // single TX
        // -> All RX
        int rx_identifier = threadnum;
        std::vector<std::shared_ptr<std::ofstream>> outfiles;
        for (size_t i = 0; i < buffs.size(); i++) {
            // rx_identifier * 2 + i in order to get correct channel number in filename
            const std::string this_filename = generateRxFilename(RA_rx_file,
                rx_identifier * 2 + i,
                RA_singleTX,
                0,
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
        
        rx_streamer->issue_stream_cmd(stream_cmd);
        const auto start_time = std::chrono::steady_clock::now();
        const auto stop_time =
        start_time + std::chrono::milliseconds(int64_t(1000 * RA_time_requested+1000*RA_delay_start_time));
        // Track time and samps between updating the BW summary
        auto last_update                     = start_time;
        unsigned long long last_update_samps = 0;
        int loop_num = 0;
        while (not RA_stop_signal_called
           and (RA_nsamps >= num_total_samps or RA_nsamps == 0)
           and (RA_time_requested == 0.0 or std::chrono::steady_clock::now() <= stop_time)) {
            const auto now = std::chrono::steady_clock::now();
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
            if(bw_summary){
                last_update_samps += num_rx_samps;
                const auto time_since_last_update = now - last_update;
                if (time_since_last_update > std::chrono::seconds(1)) {
                    const double time_since_last_update_s =
                        std::chrono::duration<double>(time_since_last_update).count();
                    const double rate = double(last_update_samps) / time_since_last_update_s;
                    std::cout << "\t" << (rate / 1e6) << " Msps" << std::endl;
                    last_update_samps = 0;
                    last_update       = now;
                }
            }
        }
        const auto actual_stop_time = std::chrono::steady_clock::now();

        // Shut down receiver
        stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
        rx_streamer->issue_stream_cmd(stream_cmd);

        for (size_t i = 0; i < outfiles.size(); i++) {
            outfiles[i]->close();
        }
        if (stats) {
            std::cout << std::endl;
            if (RA_nsamps > 0){
                std::cout << num_total_samps << " Samples Recieved: rerun with timed run for accurate stats." << std::endl;
               return;
            }
            const double actual_duration_seconds =
                std::chrono::duration<float>(actual_stop_time - start_time).count()-RA_delay_start_time;
            size_t adjusted_samples = num_total_samps/rx_streamer->get_num_channels();
            std::cout << std::endl;
            std::cout << boost::format("Thread: %d Received %d samples in %f seconds") % threadnum % num_total_samps
                            % actual_duration_seconds
                    << std::endl;
            const double rate = (double)adjusted_samples / actual_duration_seconds;
            std::cout << (rate / 1e6) << " Msps" << std::endl;

        }
    }
};
/***********************************************************************
 * Main function
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // find configuration file -cfgFile adds to "desc" variable
    Arch_multifreq_loopback usrpSystem(argc, argv);
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
    // INFO: Comment what each initialization does what type of data is stored in each.
    usrpSystem.localTime();
    
    
    std::signal(SIGINT, usrpSystem.sigIntHandler);
    double freq = 1000000000;
    while (freq <= 5500000000 and not usrpSystem.RA_stop_signal_called) {

        usrpSystem.tuneRX(freq);
        usrpSystem.tuneTX(freq);
        usrpSystem.updateDelayedStartTime();
        usrpSystem.transmitFromReplay();
        usrpSystem.spawnReceiveThreads();
        // Join Threads
        usrpSystem.joinAllThreads();
        freq += 100000000;
    }
    
    std::signal(SIGINT, SIG_DFL);
    std::cout << "Run complete." << std::endl;
    usrpSystem.stopReplay();
    // Kill LO
    usrpSystem.killLOs();
    

    std::cout << std::endl << "Closing USRP Sessions" << std::endl << std::endl;
    return EXIT_SUCCESS;
}