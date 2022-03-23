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
#include <fcntl.h>
#include <fstream>
#include <memory>
#include <thread>

#include <chrono>

#define NumberOfTriesToMake 5000

class Pipe_Example : public RefArch{
public:
    using RefArch::RefArch;
    std::string pipe_folder_location;
    int pipe_file_buffer_size;
    std::vector<std::shared_ptr<PipeFile>> outfiles;
    int32_t maximum_number_of_samples =0;

    /**
     * @brief Used to add options to the configuration file.
     * 
     */
    void addAditionalOptions() override{
        namespace po = boost::program_options;
        RA_desc.add_options()
        ("PipeFolderLocation",
            po::value<std::string>(&pipe_folder_location)->required(), 
            "Absolute location of PipeFile")
        ("PipeFileBufferSize",
            po::value<int>(&pipe_file_buffer_size)->required(), 
            "Number of samples to read or write to file");
    }

    /**
     * @brief Copied out the transmitFromReplay in the source code but made modifications
     *          to allow for STREAM_MODE_NUM_SAMPS_AND_MORE
     *          Also changed RA_nsamps to maximum_number_of_samples
     * 
     */
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

    /**
     * @brief is a background process that reads the data from each channel. Changed 
     *          so that it uses STREAM_MODE_NUM_SAMPS_AND_MORE and will end if 
     *          maximum_number_of_samples <=0.
     *          Includes the write to pipe implimentation.
     *          This will read the number of samples required into memory THEN write
     *          to the pipe.
     * 
     * @param rx_channel_nums number of rx channels
     * @param threadnum The thread number
     * @param rx_streamer sptr to the rx_streamer
     */
    void recv(
       int rx_channel_nums, int threadnum, uhd::rx_streamer::sptr rx_streamer) override
    {
        std::vector<std::shared_ptr<PipeFile>> thread_files;
        for (int i =0; i<rx_channel_nums; i++){
            thread_files.push_back(outfiles[threadnum*rx_channel_nums+i]);
        }
        uhd::set_thread_priority_safe(0.9F);
        int total_num_samples_returned = 0; 
        // Prepare buffers for received samples and metadata
        std::vector<boost::circular_buffer<std::complex<short>>> buffs(
            rx_channel_nums, boost::circular_buffer<std::complex<short>>(maximum_number_of_samples + 1));
        // create a vector of pointers to point to each of the channel buffers
        std::vector<std::complex<short>*> buff_ptrs;
        for (size_t i = 0; i < buffs.size(); i++) {
            buff_ptrs.push_back(&buffs[i].front());
        }
        bool overflow_message = true;
        // setup streaming
        uhd::rx_metadata_t md;
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE);
        stream_cmd.num_samps  = std::max(
                                    thread_files[0]->returnedValues()[0],
                                    thread_files[1]->returnedValues()[0]);
        
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
        typedef std::chrono::high_resolution_clock Clock;
        auto overTime = Clock::now(); 
        while (not RA_stop_signal_called
               and stream_cmd.num_samps > total_num_samples_returned and !stream_cmd.num_samps == 0) 
               {
            auto TempTime = Clock::now();
            printf("%d::Receieve\n",threadnum);
            size_t samps_retuned = rx_streamer->recv(buff_ptrs, stream_cmd.num_samps, md, RA_rx_timeout);
            loop_num += 1;
            printf("%d::Error check\n",threadnum);
            if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
                std::cout << boost::format("Timeout while streaming") << std::endl <<std::flush;
                break;
            }
            if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
                if (overflow_message) {
                    overflow_message    = false;
                    std::string tempstr = "\n thread:" + std::to_string(threadnum) + '\n'
                                          + "loop_num:" + std::to_string(loop_num) + '\n';
                    std::cout << tempstr <<std::flush;
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
                printf("%d::Got Error\n",threadnum);
                continue;
            }
            if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
                throw std::runtime_error(
                    str(boost::format("Receiver error %s") % md.strerror()));
                    break;
            }
            // Write to Pipe Implimentation
            total_num_samples_returned += samps_retuned;
            int buffer_number=0;
            
            int total_sent = 0;
            for (auto file : thread_files) {
                printf("%d::Write to file\n",threadnum);
                // sets the samples_remaining to either the number recieved from rev (samps_returned)
                // or sets them to an ammount that gets us to the requested number of samples
                size_t samples_remaining = 
                    file->returnedValues()[0] >= total_num_samples_returned ? samps_retuned : 
                    samps_retuned - (total_num_samples_returned - file->returnedValues()[0]);
                int file_write_index = 0;
                int number_of_tries = NumberOfTriesToMake; 
                uint iterations = 0;                     // used for benchmarking
                double combined_rate_in_miliseconds = 0; // used for benchmarking
                auto timer = Clock::now();               // used for benchmarking
                while(samples_remaining > 0 && !RA_stop_signal_called){
                    int num_of_bytes_to_write = std::min(samples_remaining * sizeof(std::complex<short>), size_t(pipe_file_buffer_size));
                    std::complex<short>* it = buff_ptrs[buffer_number]+file_write_index;
                    int returned_num_bytes = file->writeFile(it,num_of_bytes_to_write);
                    if(returned_num_bytes!=-1){
                        file_write_index += returned_num_bytes/sizeof(std::complex<short>);
                        samples_remaining -= returned_num_bytes/sizeof(std::complex<short>);
                        //Code used to benchmark the write rate. Just benchmarking first thread
                        //if (threadnum ==0 && buffer_number==0){
                            auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-timer).count();
                            iterations++;
                            deltaTime = deltaTime == 0 ? 1 : deltaTime;
                            combined_rate_in_miliseconds += (returned_num_bytes/sizeof(std::complex<short>))*1000/deltaTime;
                            total_sent += returned_num_bytes/sizeof(std::complex<short>);
                            timer = Clock::now();
                        //}
                        number_of_tries=NumberOfTriesToMake;
                    }
                    else if (number_of_tries <=0) {
                                std::cout << std::endl
                                    <<"#######Broke Early########\n"
                                    <<"                   Thread:"<<threadnum<< std::endl
                                    <<"               FileNumber:"<<buffer_number<< std::endl
                                    <<"        Samples Remaining:"<<samples_remaining<< std::endl
                                    <<"Number of Samples Written:"<<file_write_index<<std::endl
                                    <<"##########################"<<std::endl;
                        break;
                    }
                    //Try to write again. Waiting a small delay.
                    else {
                        --number_of_tries;
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                }
                //Code used to benchmark the write rate. Just benchmarking first thread
                //if (threadnum ==0 && buffer_number==0){
                    auto deltaTimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-overTime).count();
                    _Float64 speed=0;
                    iterations = iterations == 0 ? 1 : iterations;
                    if (iterations>0) speed=(combined_rate_in_miliseconds/iterations);
                    std::cout   <<"returned_num_bytes: "<<file_write_index<<std::endl
								<<" Samples remaining: "<<samples_remaining<<std::endl
                                <<"        Total Time: "<<deltaTimeMS<<std::endl
                                <<"         Rate KS/s:"<<speed<<std::endl
                                <<"        iterations:"<<iterations<<std::endl
                                <<"             total:"<<total_sent<<std::endl;//}
                ++buffer_number;
            }
        }
        //End of recv
        for (auto file : thread_files) {
            printf("%d::closingFiles\n",threadnum);
            file->closeFile();
        }
    }

    /**
     * @brief Create a Pipes object at file location
     * 
     */
    void createPipes(){
        for (size_t i = 0; i < RA_rx_stream_vector.size(); i++) {
            const std::string this_filename = pipe_folder_location+"/"+std::to_string(i)+".fifo";
            auto Pipe = std::shared_ptr<PipeFile>(new PipeFile(this_filename));
            outfiles.push_back(Pipe);
        }
    }

    /**
     * @brief Creates a test file at the pipe_folder_location to make sure we are able to
     *          set the buffer size.
     * 
     */
    void testFileSize(){
        const std::string this_filename = pipe_folder_location+"/"+"test.fifo";
        PipeFile read_pipe(this_filename);
        PipeFile write_pipe(this_filename);
        write_pipe.openFile(O_WRONLY);
        read_pipe.openFile(O_RDONLY);
        bool all_returned;
        do{
            all_returned = true;
            all_returned &= write_pipe.isFileOpen();
            all_returned &= read_pipe.isFileOpen();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }while(!RA_stop_signal_called && !all_returned); 
        int file_size_buffer = write_pipe.setfileSize(pipe_file_buffer_size);
        if(file_size_buffer != pipe_file_buffer_size){
            UHD_LOGGER_ERROR("Configuration File")
                 << (boost::format(
                     " Could not set buffer size to %d. Instead set to %d \n"
                     " Try setting permissions of this executable with this command\n "
                     " sudo setcap 'CAP_SYS_RESOURCE=+ep' Arch_pipe_example\n "
                     " Or by reducing the pipe file buffer size inside the configuration file. ")
                     % pipe_file_buffer_size %file_size_buffer);
            throw std::runtime_error("Configuration File: Buffer size could not be set");
        }
    }

    /**
     * @brief Spawns background processes to read from the pipe. If all files return
     *          OR the the RA_stop_signal_called is true it will return. Stores
     *          data inside the maximum_number_of_samples. Initially sets 
     *          maximum_number_of_samples to 0;
     * 
     * @param pollRateMs rate to poll for all returned samples.
     */
    void readPipes(int pollRateMs){
        for(auto pipe : outfiles){
            pipe->readSamplesNonBlocking(1);
            printf("Calling readSamplesNonBlocking\n");
             }
        bool all_returned;
        maximum_number_of_samples=0;
        int index_not_returned=0; //This is done so we don't traverse over the returned ones
        do{
            all_returned = true;
            for(; index_not_returned < outfiles.size(); index_not_returned++){
                all_returned &= outfiles[index_not_returned]->didBackgroundReturn();
                if(all_returned){
                    maximum_number_of_samples = std::max(maximum_number_of_samples,
                        outfiles[index_not_returned]->returnedValues()[0]); 
                }
                else {break;}
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(pollRateMs));
        }while(!RA_stop_signal_called && !all_returned); 
        for(auto pipe : outfiles){ pipe->closeFile();}
    }

    void openPipesForWriting(int pollRateMs){
        for (auto outfile : outfiles){
            outfile->openFile(O_WRONLY);
        }
        bool all_returned;
        int index_not_returned=0; //This is done so we don't traverse over the returned ones
        do{
            all_returned = true;
            for(; index_not_returned < outfiles.size(); index_not_returned++){
                all_returned &= outfiles[index_not_returned]->isFileOpen();
                if(!all_returned){break;}
            }
            std::cout<<"index1:"<<index_not_returned<<std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(pollRateMs));
        }while(!RA_stop_signal_called && !all_returned); 
        if(all_returned){
            for (auto outfile : outfiles){
                outfile->setfileSize(pipe_file_buffer_size); //todo: Need error handeler
            }
        }
    }
};

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // find configuration file -cfgFile adds to "desc" variable
    Pipe_Example usrpSystem(argc, argv);
    usrpSystem.parseConfig();

    usrpSystem.testFileSize();
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
    // Create pipes
    usrpSystem.createPipes();
    std::signal(SIGINT, usrpSystem.sigIntHandler);
    do{
        usrpSystem.readPipes(100);
        if(usrpSystem.maximum_number_of_samples!=0){
            usrpSystem.openPipesForWriting(100);
            usrpSystem.updateDelayedStartTime();
            usrpSystem.transmitFromReplay();
            usrpSystem.spawnReceiveThreads();
            usrpSystem.joinAllThreads();
            std::cout << " DONE  GENERATING: "<<std::endl<<std::endl<<std::endl;
        }
        else usrpSystem.RA_stop_signal_called=true;
        usrpSystem.stopReplay();
        std::cout<<"END:"<<usrpSystem.maximum_number_of_samples<<std::endl<<std::flush;
    }while(!usrpSystem.RA_stop_signal_called);
    
    std::signal(SIGINT, SIG_DFL);
    std::cout << "Run complete." << std::endl;
    // Kill Replay
    // Kill LO
    usrpSystem.killLOs();


    std::cout << std::endl << "Closing USRP Sessions" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
