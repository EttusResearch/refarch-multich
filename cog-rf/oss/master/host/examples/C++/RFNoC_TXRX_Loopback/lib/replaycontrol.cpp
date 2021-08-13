#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <boost/thread/thread.hpp>
#include <boost/atomic.hpp>
#include <boost/array.hpp>
#include <fstream>
#include <csignal>

#include "structures.hpp"
#include "receivefunctions.hpp"
#include "replaycontrol.hpp"
#include "recvtofile.hpp"
#include "blocksettings.hpp"
#include "sync.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>

#include "multithread.hpp"

/************************************************************************************
Import Data into USRP Replay Blocks
For now, this imports the same data into each replay block. Eventually multi-file capability
may be added. 
*************************************************************************************/

int ReplayControl::importData(GraphSettings& graphSettings, SignalSettings& signalSettings){

    //TODO: Make Changes for single loopback
    
    

    // Constants related to the Replay block
    const size_t replay_word_size = 8; // Size of words used by replay block
    const size_t sample_size      = 4; // Complex signed 16-bit is 32 bits per sample
    const size_t samples_per_word = 2; // Number of sc16 samples per word

    /************************************************************************
     * Read the data to replay
    ***********************************************************************/
    // Open the file
    std::ifstream infile(signalSettings.file.c_str(), std::ifstream::binary);
    if (!infile.is_open()) {
        std::cerr << "Could not open specified file" << std::endl;
        return EXIT_FAILURE;
    }

    // Get the file size
    infile.seekg(0, std::ios::end);
    size_t file_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    // Calculate the number of 64-bit words and samples to replay
    size_t words_to_replay   = file_size / replay_word_size;
    signalSettings.samples_to_replay = file_size / sample_size;
    //std::cout << "FILESIZE:  " << file_size << std::endl;

    graphSettings.replay_buff_addr = 0;
    graphSettings.replay_buff_size = signalSettings.samples_to_replay * sample_size;

    // Create buffer
    std::vector<char> tx_buffer(signalSettings.samples_to_replay * sample_size);
    char* tx_buf_ptr = &tx_buffer[0];

    // Read file into buffer, rounded down to number of words
    infile.read(tx_buf_ptr, signalSettings.samples_to_replay * sample_size);
    infile.close();

    if (signalSettings.singleTXRX_loopback == false){

        

        for (int i = 0; i < graphSettings.replay_ctrls.size(); i = i + 2){

            

        


            /************************************************************************
             * Configure replay block
            ***********************************************************************/
            // Configure a buffer in the on-board memory at address 0 that's equal in
            // size to the file we want to play back (rounded down to a multiple of
            // 64-bit words). Note that it is allowed to playback a different size or
            // location from what was recorded.
            
            std::cout << graphSettings.replay_ctrls[i]->get_block_id() << std::endl;
        
            graphSettings.replay_ctrls[i]->record(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, 0);

            // Display replay configuration
            std::cout << "Replay file size:     " << graphSettings.replay_buff_size << " bytes (" << words_to_replay
                << " qwords, " << signalSettings.samples_to_replay << " samples)" << std::endl;

            std::cout << "Record base address:  0x" << std::hex
                << graphSettings.replay_ctrls[i]->get_record_offset(0) << std::dec << std::endl;
            std::cout << "Record buffer size:   " << graphSettings.replay_ctrls[i]->get_record_size(0)
                << " bytes" << std::endl;
            std::cout << "Record fullness:      " << graphSettings.replay_ctrls[i]->get_record_fullness(0)
                << " bytes" << std::endl
                << std::endl;

            // Restart record buffer repeatedly until no new data appears on the Replay
            // block's input. This will flush any data that was buffered on the input.
            uint32_t fullness;
            std::cout << "Emptying record buffer..." << std::endl;
            do {
                std::chrono::system_clock::time_point start_time;
                std::chrono::system_clock::duration time_diff;

                graphSettings.replay_ctrls[i]->record_restart(0);

                // Make sure the record buffer doesn't start to fill again
                start_time = std::chrono::system_clock::now();
                do {
                    fullness = graphSettings.replay_ctrls[i]->get_record_fullness(0);
                    if (fullness != 0)
                    std::cout << "BREAK" << std::endl;
                        break;
                    time_diff = std::chrono::system_clock::now() - start_time;
                    time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);
                } while (time_diff.count() < 250);
            } while (fullness);
            std::cout << "Record fullness:      " << graphSettings.replay_ctrls[i]->get_record_fullness(0)
                << " bytes" << std::endl
                << std::endl;

            
            

            /************************************************************************
             * Send data to replay (record the data)
            ***********************************************************************/
            std::cout << "Sending data to be recorded..." << std::endl;
            
            
            graphSettings.tx_md.start_of_burst = true;
            graphSettings.tx_md.end_of_burst   = true;
            
            
            size_t num_tx_samps  = graphSettings.tx_stream_vector[i]->send(tx_buf_ptr, signalSettings.samples_to_replay, graphSettings.tx_md);
            
            if (num_tx_samps != signalSettings.samples_to_replay) {
                std::cout << "ERROR: Unable to send " << signalSettings.samples_to_replay << " samples" << std::endl;
                return EXIT_FAILURE;
            }
            
            /************************************************************************
             * Wait for data to be stored in on-board memory
            ***********************************************************************/
            std::cout << "Waiting for recording to complete..." << std::endl;
            
            while (graphSettings.replay_ctrls[i]->get_record_fullness(0) < graphSettings.replay_buff_size);
            std::cout << "Record fullness:      " << graphSettings.replay_ctrls[i]->get_record_fullness(0)
                << " bytes" << std::endl
                << std::endl;
                
        
            
        }

    }
    else{

        
        //Single TX/RX Loopback

        /************************************************************************
         * Configure replay block
        ***********************************************************************/
        // Configure a buffer in the on-board memory at address 0 that's equal in
        // size to the file we want to play back (rounded down to a multiple of
        // 64-bit words). Note that it is allowed to playback a different size or
        // location from what was recorded.
        
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << std::endl;
    
        graphSettings.replay_ctrls[signalSettings.singleTX]->record(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, 0);

        // Display replay configuration
        std::cout << "Replay file size:     " << graphSettings.replay_buff_size << " bytes (" << words_to_replay
            << " qwords, " << signalSettings.samples_to_replay << " samples)" << std::endl;

        std::cout << "Record base address:  0x" << std::hex
            << graphSettings.replay_ctrls[signalSettings.singleTX]->get_record_offset(0) << std::dec << std::endl;
        std::cout << "Record buffer size:   " << graphSettings.replay_ctrls[signalSettings.singleTX]->get_record_size(0)
            << " bytes" << std::endl;
        std::cout << "Record fullness:      " << graphSettings.replay_ctrls[signalSettings.singleTX]->get_record_fullness(0)
            << " bytes" << std::endl
            << std::endl;

        // Restart record buffer repeatedly until no new data appears on the Replay
        // block's input. This will flush any data that was buffered on the input.
        uint32_t fullness;
        std::cout << "Emptying record buffer..." << std::endl;
        do {
            std::chrono::system_clock::time_point start_time;
            std::chrono::system_clock::duration time_diff;

            graphSettings.replay_ctrls[signalSettings.singleTX]->record_restart(0);

            // Make sure the record buffer doesn't start to fill again
            start_time = std::chrono::system_clock::now();
            do {
                fullness = graphSettings.replay_ctrls[signalSettings.singleTX]->get_record_fullness(0);
                if (fullness != 0)
                std::cout << "BREAK" << std::endl;
                    break;
                time_diff = std::chrono::system_clock::now() - start_time;
                time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);
            } while (time_diff.count() < 250);
        } while (fullness);
        std::cout << "Record fullness:      " << graphSettings.replay_ctrls[signalSettings.singleTX]->get_record_fullness(0)
            << " bytes" << std::endl
            << std::endl;

        
        

        /************************************************************************
         * Send data to replay (record the data)
        ***********************************************************************/
        std::cout << "Sending data to be recorded..." << std::endl;
        
        
        graphSettings.tx_md.start_of_burst = true;
        graphSettings.tx_md.end_of_burst   = true;
        
        
        size_t num_tx_samps  = graphSettings.tx_stream_vector[signalSettings.singleTX]->send(tx_buf_ptr, signalSettings.samples_to_replay, graphSettings.tx_md);
        
        if (num_tx_samps != signalSettings.samples_to_replay) {
            std::cout << "ERROR: Unable to send " << signalSettings.samples_to_replay << " samples" << std::endl;
            return EXIT_FAILURE;
        }
        
        /************************************************************************
         * Wait for data to be stored in on-board memory
        ***********************************************************************/
        std::cout << "Waiting for recording to complete..." << std::endl;
        
        while (graphSettings.replay_ctrls[signalSettings.singleTX]->get_record_fullness(0) < graphSettings.replay_buff_size);
        std::cout << "Record fullness:      " << graphSettings.replay_ctrls[signalSettings.singleTX]->get_record_fullness(0)
            << " bytes" << std::endl
            << std::endl;
            


    }

    return EXIT_SUCCESS;


}



int ReplayControl::runTXRXiterativeloopback(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings){

    /*******************************************************************************************************************************
    This function iterates over each TX, transmitting to all RXs. A single timed command is sent to the USRP for each TX and each RX 
    due to command queue limitations. A function that utilizes the command queue is in work and requires continuous RX streaming.
    There are various options to this function, see the documentation for more details. 

    ********************************************************************************************************************************/

     //Make a vector of rx channels from 0 to n
    std::vector<size_t> rx_channel_nums;
    for (size_t chan = 0; chan< graphSettings.radio_ctrls.size(); chan++){
        rx_channel_nums.push_back(chan);
        
    }

    /************************************************************************
     * Start replay of data
     ***********************************************************************/
    for (int run_num = 0; run_num < signalSettings.nruns; run_num++){

        stop_signal_called = false;
        int tx_chan_num = 0;

        
        //Adjust times of tx/rx for looped and iterative configurations
        //Determine what the timestamp of first transmission in a cycle should be.
        if (run_num > 0){
            
            uhd::time_spec_t now = graphSettings.graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now();
            graphSettings.time_spec = uhd::time_spec_t(now + signalSettings.rep_delay);

        }
        else{
            
            uhd::time_spec_t now = graphSettings.graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now();
            graphSettings.time_spec = uhd::time_spec_t(now + signalSettings.rtime);

        }

        graphSettings.time_adjustment = uhd::time_spec_t(signalSettings.time_adjust);

        
        std::cout << "TX in Iterative Mode" << std::endl;

        for (int i_play = 0 ; i_play < graphSettings.replay_ctrls.size() ; i_play++){

            //adjust time for replay blocks, first replay of next loop is delayed but all that follow are relative to the first
            //This also accounts for the case where there is no delay between consecutive runs. There is always a min delay of signalSettings.rtime due to the 
            //nature of synchronization. 
            if ((i_play > 0 and run_num > 0) or signalSettings.rep_delay == 0){
                
                uhd::time_spec_t now = graphSettings.graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now();
                graphSettings.time_spec = uhd::time_spec_t(signalSettings.rtime + now);
                
            }

            

            // Replay nsamps, wrapping back to the start of the buffer if nsamps is
            // larger than the buffer size.
            graphSettings.replay_ctrls[i_play]->config_play(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, graphSettings.replay_chan_vector[i_play]);
            
            uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
            stream_cmd.num_samps = signalSettings.nsamps;
            std::cout << graphSettings.replay_ctrls[i_play]->get_block_id() << " Port: " << graphSettings.replay_chan_vector[i_play] <<  std::endl;
            std::cout << graphSettings.replay_ctrls[i_play]->get_block_id() << " Issuing replay command for " << signalSettings.nsamps << " samps..." << std::endl;
            stream_cmd.stream_now = false;
            stream_cmd.time_spec = graphSettings.time_spec;
            graphSettings.replay_ctrls[i_play]->issue_stream_cmd(stream_cmd, graphSettings.replay_chan_vector[i_play]);


            if (signalSettings.format == "sc16"){
            recvToFile<std::complex<short>>(
                            graphSettings.rx_stream, signalSettings.format, signalSettings.otw, signalSettings.rx_file, signalSettings.spb, signalSettings.nsamps, graphSettings.time_spec, 
                            rx_channel_nums, signalSettings.rx_timeout, deviceSettings.rx_rate, tx_chan_num, signalSettings, run_num, deviceSettings, graphSettings, signalSettings.time_requested); //rx_usrp
            }
            else {
                    
                    throw std::runtime_error("Unknown type " + signalSettings.format);
            }
        
            graphSettings.time_spec += graphSettings.time_adjustment;
            tx_chan_num++;
            


        }

            
            stop_signal_called = true;
        
    
    }
    


    return EXIT_SUCCESS;
}

void ReplayControl::stopReplay(GraphSettings& graphSettings){

     /************************************************************************
     * Issue stop command
     ***********************************************************************/
    std::cout  << "Stopping replay..." << std::endl;
    
    for (int i_kill = 0; i_kill < graphSettings.replay_ctrls.size(); i_kill++){
        graphSettings.replay_ctrls[i_kill]->stop(graphSettings.replay_chan_vector[i_kill]);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
    }

    


}

void ReplayControl::replayTransmit(GraphSettings& graphSettings, SignalSettings& signalSettings){
    //Individual channel replay transmit
    //TBD


    uhd::time_spec_t now = graphSettings.graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now();
    graphSettings.time_spec = uhd::time_spec_t(now + signalSettings.rtime);

    if (signalSettings.nsamps <= 0){
        //replay the entire buffer over and over
        std::cout << "Issuing replay command for " << signalSettings.samples_to_replay << " samps in continuous mode..." << std::endl;
        // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
        graphSettings.replay_ctrls[signalSettings.singleTX]->config_play(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.num_samps = signalSettings.nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = graphSettings.time_spec;
        graphSettings.replay_ctrls[signalSettings.singleTX]->issue_stream_cmd(stream_cmd, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
    }
    else{
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << " Port: " << graphSettings.replay_chan_vector[signalSettings.singleTX] <<  std::endl;
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << " Issuing replay command for " << signalSettings.nsamps << " samps..." << std::endl;

         // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
        graphSettings.replay_ctrls[signalSettings.singleTX]->config_play(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        stream_cmd.num_samps = signalSettings.nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = graphSettings.time_spec;
        graphSettings.replay_ctrls[signalSettings.singleTX]->issue_stream_cmd(stream_cmd, graphSettings.replay_chan_vector[signalSettings.singleTX]);

    }

   
}

void ReplayControl::rfnocReceive(GraphSettings& graphSettings, double startTime){
    //individual channel rfnoc receive
    //TBD

}

int ReplayControl::singleTXLoopback(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings){
    /*******************************************************************************************************************
    Channel to Channel Loopback
    User has the option af a single TX -> ALL RX or a single TX to a single RX. 
    If the user sets the number of samples to zero, this function will stream continuously. WARNING: This can quickly 
    create large files. 
    *******************************************************************************************************************/
    

    std::vector<size_t> rx_channel_nums;
    //thread group for multithreading
    boost::thread_group thread_group;
    
    if (signalSettings.singleTXRX_loopback == true){
        rx_channel_nums.push_back(signalSettings.singleRX);
       
        std::cout << "Single RX" << std::endl;
    }
    else{

        for (size_t chan = 0; chan< graphSettings.radio_ctrls.size(); chan++){
            rx_channel_nums.push_back(chan);
        
        }

    }
        


    uhd::time_spec_t now = graphSettings.graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now();
    graphSettings.time_spec = uhd::time_spec_t(now + signalSettings.rtime);

    if (signalSettings.nsamps <= 0){
        //replay the entire buffer over and over
        std::cout << "Issuing replay command for " << signalSettings.samples_to_replay << " samps in continuous mode..." << std::endl;
        // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
        std::cout << "Replaying data (Press Ctrl+C to stop)..." << std::endl;

        graphSettings.replay_ctrls[signalSettings.singleTX]->config_play(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.num_samps = signalSettings.nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = graphSettings.time_spec;
        graphSettings.replay_ctrls[signalSettings.singleTX]->issue_stream_cmd(stream_cmd, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
    }
    else{
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << " Port: " << graphSettings.replay_chan_vector[signalSettings.singleTX] <<  std::endl;
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << " Issuing replay command for " << signalSettings.nsamps << " samps..." << std::endl;

         // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
        graphSettings.replay_ctrls[signalSettings.singleTX]->config_play(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        stream_cmd.num_samps = signalSettings.nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = graphSettings.time_spec;
        graphSettings.replay_ctrls[signalSettings.singleTX]->issue_stream_cmd(stream_cmd, graphSettings.replay_chan_vector[signalSettings.singleTX]);

    }

    
    //Receive
    if (signalSettings.format == "sc16"){
    recvToFile<std::complex<short>>(
                    graphSettings.rx_stream, signalSettings.format, signalSettings.otw, signalSettings.rx_file, signalSettings.spb, signalSettings.nsamps, graphSettings.time_spec, 
                    rx_channel_nums, signalSettings.rx_timeout, deviceSettings.rx_rate, signalSettings.singleTX, signalSettings, 0, deviceSettings,  graphSettings, signalSettings.time_requested); //rx_usrp
    }
    else {
            
            throw std::runtime_error("Unknown type " + signalSettings.format);
    }

        
    //If running in continuous mode, call signal handler if user says to stop.     
    if (signalSettings.nsamps <= 0){
        std::signal(SIGINT, &ReplayControl::sig_int_handler);
        std::cout << "Replaying data (Press Ctrl+C to stop)..." << std::endl;

        while (not stop_signal_called)
            ;

        // Remove SIGINT handler
        std::signal(SIGINT, SIG_DFL);
    }
    else{
        stop_signal_called = true;
    }

    
    return EXIT_SUCCESS;
        
    }

int ReplayControl::multiCWloopback(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd){

    //iterates each TX, changing carrier waves after every TX has transmitted. 
    
    for (double freq = 2300000000; freq <= 2400000000; freq += 100000000){

        deviceSettings.tx_freq = freq;
        
        deviceSettings.rx_freq = freq;
        //Tune RX
        BlockSettings::tuneRX(graphSettings, deviceSettings, pmd);
        //Tune TX
        BlockSettings::tuneTX(graphSettings, deviceSettings, pmd);

         //Check RX Sensor Lock
        SyncDevices::checkRXSensorLock(graphSettings);
        //Check TX Sensor Lock
        SyncDevices::checkTXSensorLock(graphSettings);

        // Allow for some setup time
        //std::this_thread::sleep_for(std::chrono::seconds(0));
        //Begin TX and RX
        ReplayControl::runTXRXiterativeloopback(graphSettings, signalSettings, deviceSettings);
        //Kill Replay 
        ReplayControl::stopReplay(graphSettings);
    }
    

    return EXIT_SUCCESS;


    }

void ReplayControl::runSelector(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd ){

    if (deviceSettings.mode == 0){
        ReplayControl::singleTXLoopback( graphSettings, signalSettings, deviceSettings);
    }
    else if(deviceSettings.mode == 1){
        signalSettings.singleTXRX_loopback = true;
        ReplayControl::singleTXLoopback( graphSettings, signalSettings, deviceSettings);
    }
    else if(deviceSettings.mode == 2){
        ReplayControl::runTXRXiterativeloopback(graphSettings, signalSettings, deviceSettings);

    }
    else if(deviceSettings.mode == 3){
        ReplayControl::multiCWloopback(graphSettings, signalSettings, deviceSettings, pmd);
    }
    else if(deviceSettings.mode == 4){
        ReplayControl::singleTXLoopbackMultithread( graphSettings, signalSettings, deviceSettings);

    }
    else if(deviceSettings.mode == 5){
        //Seperate threads for read and write 
        //ReplayControl::multithreadLoopback(graphSettings, signalSettings, deviceSettings);
    }

}

void ReplayControl::sig_int_handler(int)
{
    stop_signal_called = true;
} 


int ReplayControl::singleTXLoopbackMultithread(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings){
    /*******************************************************************************************************************
    Channel to Channel Loopback
    User has the option af a single TX -> ALL RX or a single TX to a single RX. 
    If the user sets the number of samples to zero, this function will stream continuously. WARNING: This can quickly 
    create large files. 
    *******************************************************************************************************************/
   

    std::vector<size_t> rx_channel_nums;
    //thread group for multithreading
    boost::thread_group thread_group;
    
    

        
    rx_channel_nums.push_back(0);
        
        

        


    uhd::time_spec_t now = graphSettings.graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now();
    graphSettings.time_spec = uhd::time_spec_t(now + signalSettings.rtime);

     //Receive graphSettings.rx_stream_vector.size()
    if (signalSettings.singleTXRX_loopback == false){
        if (signalSettings.format == "sc16"){
            for (int i = 0; i < graphSettings.rx_stream_vector.size(); i++){
                std::cout << "Spawning RX Thread.." << i << std::endl;
                thread_group.create_thread(std::bind(&recvToFileMultithread, graphSettings.rx_stream_vector[i], signalSettings.format, signalSettings.otw, signalSettings.rx_file, signalSettings.spb, signalSettings.nsamps, graphSettings.time_spec, 
                rx_channel_nums, signalSettings.rx_timeout, deviceSettings.rx_rate, signalSettings.singleTX, signalSettings, 0, deviceSettings,  graphSettings, signalSettings.time_requested, i ));
            }
    
        }
        else {
                
                throw std::runtime_error("Unknown type " + signalSettings.format);
        }

    }
    else{
         if (signalSettings.format == "sc16"){
            for (int i = 0; i < 1; i++){
                //std::cout << "Spawning RX Thread.." << i << std::endl;
                thread_group.create_thread(std::bind(&recvToFileMultithread, graphSettings.rx_stream_vector[signalSettings.singleRX], signalSettings.format, signalSettings.otw, signalSettings.rx_file, signalSettings.spb, signalSettings.nsamps, graphSettings.time_spec, 
                rx_channel_nums, signalSettings.rx_timeout, deviceSettings.rx_rate, signalSettings.singleTX, signalSettings, 0, deviceSettings,  graphSettings, signalSettings.time_requested, i ));
            }
    
        }
        else {
                
                throw std::runtime_error("Unknown type " + signalSettings.format);
        }

    }

    std::cout << "Replaying data (Press Ctrl+C to stop)..." << std::endl;
    if (signalSettings.nsamps <= 0){
        //replay the entire buffer over and over
        std::cout << "Issuing replay command for " << signalSettings.samples_to_replay << " samps in continuous mode..." << std::endl;
        // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
       // std::cout << "Replaying data (Press Ctrl+C to stop)..." << std::endl;

        graphSettings.replay_ctrls[signalSettings.singleTX]->config_play(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.num_samps = signalSettings.nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = graphSettings.time_spec;
        graphSettings.replay_ctrls[signalSettings.singleTX]->issue_stream_cmd(stream_cmd, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
    }
    else{
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << " Port: " << graphSettings.replay_chan_vector[signalSettings.singleTX] <<  std::endl;
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << " Issuing replay command for " << signalSettings.nsamps << " samps..." << std::endl;

         // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
        graphSettings.replay_ctrls[signalSettings.singleTX]->config_play(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        stream_cmd.num_samps = signalSettings.nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = graphSettings.time_spec;
        graphSettings.replay_ctrls[signalSettings.singleTX]->issue_stream_cmd(stream_cmd, graphSettings.replay_chan_vector[signalSettings.singleTX]);

    }

    thread_group.join_all();
    std::cout << "Complete (Press Ctrl+C to exit)..." << std::endl;
   
    

        
    //If running in continuous mode, call signal handler if user says to stop.     
    if (signalSettings.nsamps <= 0){
        std::signal(SIGINT, &sig_int_handler);
        

        while (not stop_signal_called)
            ;
        std::cout << stop_signal_called << std::endl;

        // Remove SIGINT handler
        std::signal(SIGINT, SIG_DFL);
    }
    else{
        stop_signal_called = true;
    }

    
    
    return EXIT_SUCCESS;
        
    }

/* int ReplayControl::multithreadLoopback(GraphSettings& graphSettings, SignalSettings& signalSettings, DeviceSettings& deviceSettings){

        boost::thread_group recv_threads;
        boost::thread_group write_threads;

         std::vector<size_t> rx_channel_nums;

         rx_channel_nums.push_back(0);
        /* 
        std::vector<std::vector<std::complex<short>>> buffs(160, std::vector<std::complex<short>>(signalSettings.spb));

        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue0;
        boost::atomic<bool> done0 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue1;
        boost::atomic<bool> done1 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue2;
        boost::atomic<bool> done2 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue3;
        boost::atomic<bool> done3 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue4;
        boost::atomic<bool> done4 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue5;
        boost::atomic<bool> done5 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue6;
        boost::atomic<bool> done6 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue7;
        boost::atomic<bool> done7 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue8;
        boost::atomic<bool> done8 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue9;
        boost::atomic<bool> done9 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue10;
        boost::atomic<bool> done10 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue11;
        boost::atomic<bool> done11 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue12;
        boost::atomic<bool> done12 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue13;
        boost::atomic<bool> done13 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue14;
        boost::atomic<bool> done14 (false);
        boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue15;
        boost::atomic<bool> done15 (false);
         
        std::vector<boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>>> queue_vector;
        queue_vector.reserve(32);
        std::vector<boost::atomic<bool>> done_vector;
        done_vector.reserve(32);

        for (int i = 0; i < graphSettings.radio_ctrls.size(); i++){
            //Create Vector of Queues and Atomic-Done indicators
            boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> queue;
            boost::atomic<bool> done (false);
            queue_vector.emplace_back(boost::ref(queue));
            done_vector.emplace_back(boost::ref(done));

        }

       

        

        //Adjust Time so TXRX is synchronized across all channels
        uhd::time_spec_t now = graphSettings.graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now();
        graphSettings.time_spec = uhd::time_spec_t(now + signalSettings.rtime);


        if (signalSettings.singleTXRX_loopback == false){
            if (signalSettings.format == "sc16"){
                for (int i = 0; i < graphSettings.rx_stream_vector.size(); i++){
                    std::cout << "Spawning RX Thread.." << i << std::endl;
                    recv_threads.create_thread(std::bind(&recvThread, graphSettings.rx_stream_vector[i],  signalSettings, signalSettings.nsamps, signalSettings.time_requested,  
                    graphSettings, signalSettings.spb, graphSettings.time_spec, signalSettings.format, signalSettings.otw,  rx_channel_nums,  signalSettings.rx_timeout, deviceSettings.rx_rate, i, queue_vector, done_vector));

                    std::cout << "Spawning Write Thread.." << i << std::endl;
                    write_threads.create_thread(std::bind(&writeThread,  signalSettings.rx_file, i, signalSettings.singleTX, 0,  deviceSettings, signalSettings, signalSettings.spb, queue_vector, done_vector));
                }
        
            }
            else {
                    
                    throw std::runtime_error("Unknown type " + signalSettings.format);
            }

        }
        else{
            if (signalSettings.format == "sc16"){
                for (int i = 0; i < 1; i++){
                    std::cout << "Spawning RX Thread.." << i << std::endl;
                    recv_threads.create_thread(std::bind(&recvThread, graphSettings.rx_stream_vector[signalSettings.singleRX],  signalSettings, signalSettings.nsamps, signalSettings.time_requested,  
                    graphSettings, signalSettings.spb, graphSettings.time_spec, signalSettings.format, signalSettings.otw,  rx_channel_nums,  signalSettings.rx_timeout, deviceSettings.rx_rate, i, queue_vector, done_vector));

                    std::cout << "Spawning Write Thread.." << i << std::endl;
                    write_threads.create_thread(std::bind(&writeThread,  signalSettings.rx_file, i, signalSettings.singleTX, 0,  deviceSettings, signalSettings, signalSettings.spb, queue_vector, done_vector));
        
                }
            }
            else {
                    
                    throw std::runtime_error("Unknown type " + signalSettings.format);
            }

        }
        

    std::cout << "Replaying data (Press Ctrl+C to stop)..." << std::endl;
    if (signalSettings.nsamps <= 0){
        //replay the entire buffer over and over
        std::cout << "Issuing replay command for " << signalSettings.samples_to_replay << " samps in continuous mode..." << std::endl;
        // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
       // std::cout << "Replaying data (Press Ctrl+C to stop)..." << std::endl;

        graphSettings.replay_ctrls[signalSettings.singleTX]->config_play(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.num_samps = signalSettings.nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = graphSettings.time_spec;
        graphSettings.replay_ctrls[signalSettings.singleTX]->issue_stream_cmd(stream_cmd, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
    }
    else{
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << " Port: " << graphSettings.replay_chan_vector[signalSettings.singleTX] <<  std::endl;
        std::cout << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << " Issuing replay command for " << signalSettings.nsamps << " samps..." << std::endl;

         // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
        graphSettings.replay_ctrls[signalSettings.singleTX]->config_play(graphSettings.replay_buff_addr, graphSettings.replay_buff_size, graphSettings.replay_chan_vector[signalSettings.singleTX]);
        
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        stream_cmd.num_samps = signalSettings.nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = graphSettings.time_spec;
        graphSettings.replay_ctrls[signalSettings.singleTX]->issue_stream_cmd(stream_cmd, graphSettings.replay_chan_vector[signalSettings.singleTX]);

    }

    recv_threads.join_all();
    for (int i = 0; i < done_vector.size(); i++){
        done_vector[i] = true;
    }
    write_threads.join_all();
    std::cout << "Complete (Press Ctrl+C to exit)..." << std::endl;
   
    

        
    //If running in continuous mode, call signal handler if user says to stop.     
    if (signalSettings.nsamps <= 0){
        std::signal(SIGINT, &sig_int_handler);
        

        while (not stop_signal_called)
            ;
        std::cout << stop_signal_called << std::endl;

        // Remove SIGINT handler
        std::signal(SIGINT, SIG_DFL);
    }
    else{
        stop_signal_called = true;
    }

    
    
    return EXIT_SUCCESS;
       
       
       





        

}
     */

    

    

