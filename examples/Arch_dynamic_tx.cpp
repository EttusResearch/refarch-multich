//
// Copyright 2021-2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/*******************************************************************************************************************
Full TX-RX Loopback to/from host with dynamic TX.
Change each TX function to suit the application needs.
This example demonstrates a 4 channel system (Two USRPs).
ALL TX -> ALL RX or SINGLE TX -> ALL RX.
If the user sets the number of samples to zero, this function will stream
continuously. The multithreaded version
currently has each USRP in its own thread. This version uses one RX streamer per device
and one TX streamer per channel.
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


class Arch_dynamic_tx : public RefArch
{
    using RefArch::RefArch;

public:
    std::string folder_name;
    

    void localTime()
    {
        boost::posix_time::ptime timeLocal =
            boost::posix_time::second_clock::local_time();
        std::string month   = std::to_string(timeLocal.date().month());
        month               = std::string(2 - month.length(), '0') + month;
        std::string day     = std::to_string(timeLocal.date().day());
        day                 = std::string(2 - day.length(), '0') + day;
        std::string year    = std::to_string(timeLocal.date().year());
        std::string hour    = std::to_string(timeLocal.time_of_day().hours());
        hour                = std::string(2 - hour.length(), '0') + hour;
        std::string minute  = std::to_string(timeLocal.time_of_day().minutes());
        minute              = std::string(2 - minute.length(), '0') + minute;
        std::string seconds = std::to_string(timeLocal.time_of_day().seconds());
        seconds             = std::string(2 - seconds.length(), '0') + seconds;
        folder_name =
            month + day + year + "_" + hour + minute + seconds + "_" + RA_rx_file;
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

    void transmitFromFile0(
    uhd::tx_streamer::sptr tx_streamer, uhd::tx_metadata_t metadata, int num_channels)
    {
    
    uhd::set_thread_priority_safe(0.9F);
    std::vector<std::complex<float>> buff(RA_spb);
    std::ifstream infile(RA_file.c_str(), std::ifstream::binary);
    // send data until  the signal handler gets called
    while (not metadata.end_of_burst and not RA_stop_signal_called) {
        infile.read((char*)&buff.front(), buff.size() * sizeof(int16_t));
        size_t num_tx_samps = size_t(infile.gcount() / sizeof(int16_t));
        
        metadata.end_of_burst = infile.eof();
        // send the entire contents of the buffer
        const size_t samples_sent = tx_streamer->send(&buff.front(), num_tx_samps, metadata);
         if (samples_sent != num_tx_samps) {
            UHD_LOG_ERROR("TX-STREAM",
                "The tx_stream timed out sending " << num_tx_samps << " samples ("
                                                   << samples_sent << " sent).");
            return;
        }
        // do not use time spec for subsequent packets
        metadata.has_time_spec = false;
    }
    // send a mini EOB packet
    metadata.end_of_burst = true;
    tx_streamer->send("", 0, metadata);
    infile.close();
    }
    void transmitFromFile1(
        uhd::tx_streamer::sptr tx_streamer, uhd::tx_metadata_t metadata, int num_channels)
    {
    uhd::set_thread_priority_safe(0.9F);
    std::vector<std::complex<float>> buff(RA_spb);
    std::ifstream infile(RA_file.c_str(), std::ifstream::binary);
    // send data until  the signal handler gets called
    while (not metadata.end_of_burst and not RA_stop_signal_called) {
        infile.read((char*)&buff.front(), buff.size() * sizeof(int16_t));
        size_t num_tx_samps = size_t(infile.gcount() / sizeof(int16_t));
        
        metadata.end_of_burst = infile.eof();
        // send the entire contents of the buffer
        const size_t samples_sent = tx_streamer->send(&buff.front(), num_tx_samps, metadata);
         if (samples_sent != num_tx_samps) {
            UHD_LOG_ERROR("TX-STREAM",
                "The tx_stream timed out sending " << num_tx_samps << " samples ("
                                                   << samples_sent << " sent).");
            return;
        }
        // do not use time spec for subsequent packets
        metadata.has_time_spec = false; 
    }
    // send a mini EOB packet
    metadata.end_of_burst = true;
    tx_streamer->send("", 0, metadata);
    infile.close();
    }
    void transmitFromFile2(
        uhd::tx_streamer::sptr tx_streamer, uhd::tx_metadata_t metadata, int num_channels)
    {
    uhd::set_thread_priority_safe(0.9F);
    std::vector<std::complex<float>> buff(RA_spb);
    std::ifstream infile(RA_file.c_str(), std::ifstream::binary);
    // send data until  the signal handler gets called
    while (not metadata.end_of_burst and not RA_stop_signal_called) {
        infile.read((char*)&buff.front(), buff.size() * sizeof(int16_t));
        size_t num_tx_samps = size_t(infile.gcount() / sizeof(int16_t));
        
        metadata.end_of_burst = infile.eof();
        // send the entire contents of the buffer
        const size_t samples_sent = tx_streamer->send(&buff.front(), num_tx_samps, metadata);
         if (samples_sent != num_tx_samps) {
            UHD_LOG_ERROR("TX-STREAM",
                "The tx_stream timed out sending " << num_tx_samps << " samples ("
                                                   << samples_sent << " sent).");
            return;
        }
        // do not use time spec for subsequent packets
        metadata.has_time_spec = false;     
    }
    infile.close();
    }
    void transmitFromFile3(
        uhd::tx_streamer::sptr tx_streamer, uhd::tx_metadata_t metadata, int num_channels)
    {
        uhd::set_thread_priority_safe(0.9F);
    std::vector<std::complex<float>> buff(RA_spb);;
    std::ifstream infile(RA_file.c_str(), std::ifstream::binary);
    // send data until  the signal handler gets called
    while (not metadata.end_of_burst and not RA_stop_signal_called) {
        infile.read((char*)&buff.front(), buff.size() * sizeof(int16_t));
        size_t num_tx_samps = size_t(infile.gcount() / sizeof(int16_t));

        metadata.end_of_burst = infile.eof();
        // send the entire contents of the buffer
        const size_t samples_sent = tx_streamer->send(&buff.front(), num_tx_samps, metadata);
         if (samples_sent != num_tx_samps) {
            UHD_LOG_ERROR("TX-STREAM",
                "The tx_stream timed out sending " << num_tx_samps << " samples ("
                                                   << samples_sent << " sent).");
            return;
        }
        // do not use time spec for subsequent packets
        metadata.has_time_spec = false;
    }
    infile.close();
    }

    void spawnTransmitThreads()
    {
        
        if (RA_spb == 0)
            RA_spb = RA_tx_stream_vector[0]->get_max_num_samps() * 10;
        // setup the metadata flags
        uhd::tx_metadata_t md0;
        uhd::tx_metadata_t md1;
        uhd::tx_metadata_t md2;
        uhd::tx_metadata_t md3;
        md0.start_of_burst = true;
        md0.end_of_burst   = false;
        md0.has_time_spec  = true;
        md0.time_spec      = RA_start_time;
        md1.start_of_burst = true;
        md1.end_of_burst   = false;
        md1.has_time_spec  = true;
        md1.time_spec      = RA_start_time;
        md2.start_of_burst = true;
        md2.end_of_burst   = false;
        md2.has_time_spec  = true;
        md2.time_spec      = RA_start_time;
        md3.start_of_burst = true;
        md3.end_of_burst   = false;
        md3.has_time_spec  = true;
        md3.time_spec      = RA_start_time;
        
        if (RA_TX_All_Chan == true) {
            // start transmit worker thread 0, not for use with replay block.
            std::cout << "Spawning TX thread: " << 0 << std::endl;
            std::thread tx0(
                [this](uhd::tx_streamer::sptr tx_streamer,
                    uhd::tx_metadata_t metadata,
                    int num_channels) {
                    transmitFromFile0(tx_streamer, metadata, num_channels);
                },
                RA_tx_stream_vector[0],
                md0,
                1);
            RA_tx_vector_thread.push_back(std::move(tx0));

            // start transmit worker thread 1, not for use with replay block.
            std::cout << "Spawning TX thread: " << 1 << std::endl;
            std::thread tx1(
                [this](uhd::tx_streamer::sptr tx_streamer,
                    uhd::tx_metadata_t metadata,
                    int num_channels) {
                    transmitFromFile1(tx_streamer, metadata, num_channels);
                },
                RA_tx_stream_vector[1],
                md1,
                1);
            RA_tx_vector_thread.push_back(std::move(tx1));

            // start transmit worker thread 2, not for use with replay block.
            std::cout << "Spawning TX thread: " << 2 << std::endl;
            std::thread tx2(
                [this](uhd::tx_streamer::sptr tx_streamer,
                    uhd::tx_metadata_t metadata,
                    int num_channels) {
                    transmitFromFile2(tx_streamer, metadata, num_channels);
                },
                RA_tx_stream_vector[2],
                md2,
                1);
            RA_tx_vector_thread.push_back(std::move(tx2));

            // start transmit worker thread 3, not for use with replay block.
            std::cout << "Spawning TX thread: " << 3 << std::endl;
            std::thread tx3(
                [this](uhd::tx_streamer::sptr tx_streamer,
                    uhd::tx_metadata_t metadata,
                    int num_channels) {
                    transmitFromFile3(tx_streamer, metadata, num_channels);
                },
                RA_tx_stream_vector[3],
                md3,
                1);
            RA_tx_vector_thread.push_back(std::move(tx3));

        } else {
            // start transmit worker thread, not for use with replay block.
            std::cout << "Spawning Single TX thread, Channel: " << RA_singleTX
                      << std::endl;
            std::thread tx(
                [this](uhd::tx_streamer::sptr tx_streamer,
                    uhd::tx_metadata_t metadata,
                    int num_channels) {
                    transmitFromFile(tx_streamer, metadata, num_channels);
                },
                RA_tx_stream_vector[RA_singleTX],
                md0,
                1);
            RA_tx_vector_thread.push_back(std::move(tx));
        }
    }
};
/***********************************************************************
 * Main function
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // find configuration file -cfgFile adds to "desc" variable
    Arch_dynamic_tx usrpSystem(argc, argv);
    usrpSystem.parseConfig();
    // Setup Graph with input Arguments
    usrpSystem.buildGraph();
    // Sync Device Clocks
    usrpSystem.setSources();
    // Setup Radio Blocks
    usrpSystem.buildRadios();
    // Setup DDC/DUC Blocks
    usrpSystem.buildDDCDUC();

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
    usrpSystem.buildStreamsMultithreadHostTX();
    // Connect Graph
    usrpSystem.connectGraphMultithreadHostTX();
    // Commit Graph
    usrpSystem.commitGraph();
    // Allow for some setup time
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // Sync time across devices
    usrpSystem.syncAllDevices();
    // Begin TX and RX
    // INFO: Comment what each initialization does what type of data is stored in each.
    usrpSystem.localTime();
    // Calculate startime for threads
    usrpSystem.updateDelayedStartTime();
    std::signal(SIGINT, usrpSystem.sigIntHandler);
    // Transmit via host, must be before spawning receive threads.
    usrpSystem.spawnTransmitThreads();
    // Spawn receive Threads
    usrpSystem.spawnReceiveThreads();
    // Join Threads
    usrpSystem.joinAllThreads();
    std::signal(SIGINT, SIG_DFL);
    std::cout << "Run complete." << std::endl;

    // Kill LO
    usrpSystem.killLOs();

    std::cout << std::endl << "Closing USRP Sessions" << std::endl << std::endl;
    return EXIT_SUCCESS;
}