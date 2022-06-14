//
// Copyright 2021-2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/*******************************************************************************************************************
Full TX-RX Loopback to/from host using DPDK.
ALL TX -> ALL RX or SINGLE TX -> ALL RX.
If the user sets the number of samples to zero, this function will stream
continuously. This version uses one RX streamer per channel
and one TX streamer per channel. NOTE: This has only been tested with Mellonox NICs
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
#include <uhd/usrp/multi_usrp.hpp>



class Arch_txrx_fullduplex : public RefArch
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
                threadnum,
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
    void buildStreamsMultithreadHostTX() override
{
    // Build Streams for multithreaded implementation
    // TX streams from Host, not replay.
    // Each Device gets its own RX streamer.
    // Each Channel gets its own TX streamer.

    // Constants related to the Replay block
    const size_t replay_word_size = 8; // Size of words used by replay block
    const size_t sample_size      = 4; // Complex signed 16-bit is 32 bits per sample
    const size_t samples_per_word = 2; // Number of sc16 samples per word
    uhd::device_addr_t streamer_args(RA_streamargs);
    // create a receive streamer
    // std::cout << "Samples per packet: " << spp << std::endl;
    uhd::stream_args_t stream_args(RA_format, RA_otw);
    stream_args.args = streamer_args;
    std::cout << "Using streamer args: " << stream_args.args.to_string() << std::endl;
    // One stream per channel
    for (int rx_count = 0; rx_count < RA_radio_ctrls.size(); rx_count++) {
        RA_rx_stream = RA_graph->create_rx_streamer(1, stream_args);
        RA_rx_stream_vector.push_back(RA_rx_stream);
    }
    /************************************************************************
     * Set up TX streamer from host
     ***********************************************************************/
    for (int i_s2r = 0; i_s2r < RA_radio_ctrls.size(); i_s2r = i_s2r + 1) {
        streamer_args["block_id"]   = RA_duc_ctrls[i_s2r]->get_block_id().to_string();
        streamer_args["block_port"] = std::to_string(0);
        stream_args.args            = streamer_args;
        stream_args.channels        = {0};

        RA_tx_stream =
            RA_graph->create_tx_streamer(stream_args.channels.size(), stream_args);
        size_t tx_spp = RA_tx_stream->get_max_num_samps();
        // Vector of tx streamers
        RA_tx_stream_vector.push_back(RA_tx_stream);
    }
}

void connectGraphMultithreadHostTX() override
{
    // This is the function that connects the graph for the multithreaded implementation
    // streaming from host.
    UHD_LOG_INFO("CogRF", "Connecting graph...");
    // Connect Graph
    for (size_t i = 0; i < RA_radio_ctrls.size(); i++) {
        // connect radios to ddc
        RA_graph->connect(RA_radio_block_list[i], 0, RA_ddc_ctrls[i]->get_block_id(), 0);
        std::cout << "Connected " << RA_radio_block_list[i] << " to "
                  << RA_ddc_ctrls[i]->get_block_id() << std::endl;
                   std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // Vector of streamer channels.
    for (size_t i_chan = 0; i_chan < RA_rx_stream_vector.size(); i_chan++) {
        if (i_chan % 2 == 0) {
            RA_rx_stream_chan_vector.push_back(0);
        } else {
            RA_rx_stream_chan_vector.push_back(0);
        }
    }
    for (size_t j = 0; j < RA_ddc_ctrls.size(); j++) {
        // Connect DDC to streamers
        // Reusing replay chan vector, need a vector of zeros and ones
        RA_graph->connect(RA_ddc_ctrls[j]->get_block_id(),
            0,
            RA_rx_stream_vector[j],
            RA_rx_stream_chan_vector[j]);
        std::cout << "Connected " << RA_ddc_ctrls[j]->get_block_id() << " to "
                  << RA_rx_stream_vector[j] << " Port " << RA_rx_stream_chan_vector[j]
                  << std::endl;
                   std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    int pos2 = 0;

    for (auto& rctrl : RA_radio_ctrls) {
        RA_graph->connect(
            RA_duc_ctrls[pos2]->get_block_id(), RA_duc_chan, rctrl->get_block_id(), 0);
        std::cout << "Connected " << RA_duc_ctrls[pos2]->get_block_id() << " port "
                  << RA_duc_chan << " to radio " << rctrl->get_block_id() << " port " << 0
                  << std::endl;
                   std::this_thread::sleep_for(std::chrono::milliseconds(100));

        RA_graph->connect(
            RA_tx_stream_vector[pos2], 0, RA_duc_ctrls[pos2]->get_block_id(), 0);
        std::cout << "Streamer: " << RA_tx_stream_vector[pos2] << " connected to "
                  << RA_replay_ctrls[pos2]->get_block_id() << std::endl;
                   std::this_thread::sleep_for(std::chrono::milliseconds(100));
        pos2++;
    }
}

void spawnReceiveThreads() override
{
    int threadnum = 0;


    // Receive RA_rx_stream_vector.size()
    if (RA_format == "sc16") {
        for (size_t i = 0; i < RA_rx_stream_vector.size(); i = i + 1) {
            std::cout << "Spawning RX Thread.." << threadnum << std::endl;
            std::thread t(
                [this](int threadnum, uhd::rx_streamer::sptr rx_streamer, bool bw_summary, bool stats) {
                    recv(1, threadnum, rx_streamer, bw_summary, stats);
                },
                threadnum,
                RA_rx_stream_vector[i], RA_bw_summary, RA_stats);
        
            pthread_setname_np(t.native_handle(), "rx_thread");    
            RA_rx_vector_thread.push_back(std::move(t));
            threadnum++;
        }
    } else {
        throw std::runtime_error("Unknown type " + RA_format);
    }
    return;
}
// graphassembly
void buildGraph() override
{
    // create a usrp device
    std::cout << std::endl;
    uhd::device_addrs_t device_addrs = uhd::device::find(RA_args, uhd::device::USRP);
    if (not device_addrs.empty() and device_addrs.at(0).get("type", "") == "usrp1") {
        std::cerr << "*** Warning! ***" << std::endl;
        std::cerr << "Benchmark results will be inaccurate on USRP1 due to insufficient "
                     "features.\n"
                  << std::endl;
    }
    /************************************************************************
     * Create device and block controls
     ***********************************************************************/
    // If multiple USRPs are used, they are linked into a single RFNoc graph here.
    std::cout << std::endl;
    std::cout << "Creating the RFNoC graph with args: " << RA_args << "..." << std::endl;
    RA_graph = uhd::rfnoc::rfnoc_graph::make(RA_args);
}
};
/***********************************************************************
 * Main function
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    uhd::set_thread_priority_safe();
    // find configuration file -cfgFile adds to "desc" variable
    Arch_txrx_fullduplex usrpSystem(argc, argv);
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
   
    std::signal(SIGINT, usrpSystem.sigIntHandler);
     // Calculate startime for threads
    usrpSystem.updateDelayedStartTime();
    // Transmit via replay block, must be before spawning receive threads.
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