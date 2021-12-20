//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/*************************************************************************
This examples uses pipes to interface with another application.
The example opens a number of pipes equal to the number of USRPs then
reads a u16 from each file and returns that number of samples.

single TX -> # of RX.
************************************************************/

#include "FileSystem.hpp"
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

class Pipe_Example : public RefArch{
public:
    using RefArch::RefArch;
    std::string pipe_folder_location;
    std::vector<std::shared_ptr<PipeFile>> outfiles;
    int32_t maximum_number_of_samples =0;

    void addAditionalOptions() override{
        namespace po = boost::program_options;
        RA_desc.add_options()
        ("PipeFolderLocation",
            po::value<std::string>(&pipe_folder_location)->required(), 
            "Absolute location of PipeFile");
    }

    void transmitFromReplay() override{
        if (maximum_number_of_samples <=0) return;
        std::cout << "Replaying data (Press Ctrl+C to stop)..." << std::endl;
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE);
        if (RA_TX_All_Chan == true) {
            for (int i = 0; i < RA_replay_ctrls.size(); i = i + 2) {
                // Replay Block Channel (output) 0
                std::cout << RA_replay_ctrls[i]->get_block_id()
                        << " Port: " << RA_replay_chan_vector[i] << std::endl
                        << RA_replay_ctrls[i]->get_block_id()
                        << " Issuing replay command for " << maximum_number_of_samples << " samps..."
                        << std::endl;
                RA_replay_ctrls[i]->config_play(
                    RA_replay_buff_addr, RA_replay_buff_size, RA_replay_chan_vector[i]);
                stream_cmd.num_samps  = maximum_number_of_samples;
                stream_cmd.stream_now = false;
                stream_cmd.time_spec  = RA_start_time;
                RA_replay_ctrls[i]->issue_stream_cmd(stream_cmd, RA_replay_chan_vector[i]);
                // Replay Block Channel (output) 1
                std::cout << RA_replay_ctrls[i]->get_block_id()
                        << " Port: " << RA_replay_chan_vector[i + 1] << std::endl
                        << RA_replay_ctrls[i]->get_block_id()
                        << " Issuing replay command for " << maximum_number_of_samples << " samps..."
                        << std::endl;
                RA_replay_ctrls[i]->config_play(
                    RA_replay_buff_addr, RA_replay_buff_size, RA_replay_chan_vector[i + 1]);
                stream_cmd.num_samps  = maximum_number_of_samples;
                stream_cmd.stream_now = false;
                stream_cmd.time_spec  = RA_start_time;
                RA_replay_ctrls[i]->issue_stream_cmd(
                    stream_cmd, RA_replay_chan_vector[i + 1]);
            }
        } else {
            // Single TX Channel Output
            std::cout << RA_replay_ctrls[RA_singleTX]->get_block_id()
                    << " Port: " << RA_replay_chan_vector[RA_singleTX] << std::endl
                    << RA_replay_ctrls[RA_singleTX]->get_block_id()
                    << " Issuing replay command for " << maximum_number_of_samples << " samps..."
                    << std::endl;
            RA_replay_ctrls[RA_singleTX]->config_play(
                RA_replay_buff_addr, RA_replay_buff_size, RA_replay_chan_vector[RA_singleTX]);
            stream_cmd.num_samps  = maximum_number_of_samples;
            stream_cmd.stream_now = false;
            stream_cmd.time_spec  = RA_start_time;
            RA_replay_ctrls[RA_singleTX]->issue_stream_cmd(
                stream_cmd, RA_replay_chan_vector[RA_singleTX]);
        }
    }

    void recv(
       int rx_channel_nums, int threadnum, uhd::rx_streamer::sptr rx_streamer) override
    {
        
        std::vector<std::shared_ptr<PipeFile>> thread_files;
        thread_files.push_back(outfiles[threadnum*2]);
        thread_files.push_back(outfiles[threadnum*2+1]);
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
        bool overflow_message = true;
        // setup streaming
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE);
        stream_cmd.num_samps  = std::max(
                                    thread_files[0]->returnedNumberOfSamples(),
                                    thread_files[1]->returnedNumberOfSamples());
        stream_cmd.stream_now = false;
        stream_cmd.time_spec  = RA_start_time;
        md.has_time_spec      = true;
        md.time_spec          = RA_start_time;
        if (maximum_number_of_samples <=0){
            stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
            rx_streamer->issue_stream_cmd(stream_cmd);  
            return;
        }
        rx_streamer->issue_stream_cmd(stream_cmd);
        int loop_num = 0;
        while (not RA_stop_signal_called
               and (stream_cmd.num_samps > num_total_samps or stream_cmd.num_samps == 0)) 
               {
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
            num_total_samps += num_rx_samps;
            int buffer_number=0;
            for (auto file : thread_files) {
                size_t num_of_samples_to_write = 
                    num_total_samps > file->returnedNumberOfSamples() ? num_rx_samps : 
                    num_rx_samps - (num_total_samps - file->returnedNumberOfSamples());
                file->writeFile((char*)buff_ptrs[buffer_number],
                    num_of_samples_to_write * sizeof(std::complex<short>));
                ++buffer_number;
            }
        }
        
        for (auto file : thread_files) {
            file->closeFile();
        }
    }

    void createPipes(){
        for (size_t i = 0; i < RA_rx_stream_vector.size(); i++) {
            const std::string this_filename = pipe_folder_location+"/"+std::to_string(i);
            auto Pipe = std::shared_ptr<PipeFile>(new PipeFile(this_filename));
            outfiles.push_back(Pipe);
        }
    }

    void ReadPipes(int pollRateMs){
        for(auto pipe : outfiles){
            pipe->readSampleNonBlocking();
             }
        bool allReturned;
        maximum_number_of_samples=0;
        do{
            allReturned = true;
            for(auto pipe : outfiles){
                allReturned &= pipe->returnedNumberOfSamples()>=0;
                maximum_number_of_samples = std::max(maximum_number_of_samples,
                    pipe->returnedNumberOfSamples());
            }
            std::cout<<allReturned;
            std::this_thread::sleep_for(std::chrono::milliseconds(pollRateMs));
        }while(!RA_stop_signal_called && !allReturned);
        for(auto pipe : outfiles){
                std::cout<<pipe->returnedNumberOfSamples()<<std::endl;
            }
        for(auto pipe : outfiles){ pipe->closeFile();}
    }
    

};


int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // find configuration file -cfgFile adds to "desc" variable
    Pipe_Example usrpSystem(argc, argv);
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
    // INFO: Comment what each initilization does what type of data is stored in each.
    // Sync times across threads
    usrpSystem.updateDelayedStartTime();
    usrpSystem.createPipes();
    std::signal(SIGINT, usrpSystem.sigIntHandler);
    do{
        usrpSystem.ReadPipes(100);
        usrpSystem.transmitFromReplay();
        usrpSystem.spawnReceiveThreads();
        usrpSystem.joinAllThreads();
        std::cout<<"END:"<<usrpSystem.maximum_number_of_samples<<std::endl<<std::flush;
    }while(usrpSystem.maximum_number_of_samples > 0 && usrpSystem.RA_stop_signal_called);
    
    // Transmit via replay block, must be before spawning receive threads.
    
    // Spawn receive Threads
    
    // Join Threads
    
    std::signal(SIGINT, SIG_DFL);
    std::cout << "Run complete." << std::endl;
    // Kill Replay
    usrpSystem.stopReplay();
    // Kill LO
    usrpSystem.killLOs();

    std::cout << std::endl << "Closing USRP Sessions" << std::endl << std::endl;
    return EXIT_SUCCESS;
}