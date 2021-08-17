#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/atomic.hpp>
#include <boost/array.hpp>
#include <csignal>
#include <fstream>
#include <chrono>
#include <thread>
#include <iostream>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/utils/thread.hpp>
#include "structures.hpp"
#include "receivefunctions.hpp"

#include <memory>
#include <stdio.h>
#include <cstring>


using namespace std;


/* 
boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> ptr_queue1;
boost::atomic<bool> done (false);

 */




void recvThread(uhd::rx_streamer::sptr rx_stream, SignalSettings& signalSettings, int num_requested_samples, double time_requested,  
                GraphSettings& graphSettings, size_t samps_per_buff, uhd::time_spec_t start_time, const std::string& cpu_format,
        const std::string& wire_format, std::vector<size_t> rx_channel_nums, double timeout, int rx_rate, int threadnum, std::vector<boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>>> queue_vector,std::vector<boost::atomic<bool>> done_vector){

    std::cout << "Spawning Receive Sub-thread..." << std::endl;
    uhd::set_thread_priority_safe();
    int num_total_samps = 0;

    uhd::stream_args_t stream_args(cpu_format, wire_format);
    stream_args.channels             = rx_channel_nums;

    uhd::rx_metadata_t md;
    bool overflow_message = true;

    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = num_requested_samples;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = start_time;
    md.has_time_spec = true;
    md.time_spec = start_time;

    const auto stop_time = start_time + uhd::time_spec_t(time_requested);
    //https://stackoverflow.com/questions/23183629/dynamic-generation-safe-usage-of-spsc-queues

    //std::vector<std::vector<std::complex<short>>>  buffs(1, std::vector<std::complex<short>>(samps_per_buff));

    rx_stream->issue_stream_cmd(stream_cmd);

    

    //bool time_limit = graphSettings.graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now() <= stop_time;
    
    while (not signalSettings.stop_signal_called 
            and (num_requested_samples > num_total_samps or num_requested_samples == 0) and (time_requested == 0.0 or graphSettings.graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now() <= stop_time)) {
            //std::cout << "ENTERED RECV LOOP" << std::endl;
            //need to make new buffer every loop, may be too resource heavy
            recvStruct data;
            std::vector<std::vector<std::complex<short>>> buffs(1, std::vector<std::complex<short>>(samps_per_buff));

            //PROBLEM HERE
            data.buff_ptrs.push_back(&buffs[0].front());
            //std::cout << "BUFF PTR" << std::endl;
            
            

            //std::cout << "Begining Recieve..." << std::endl; 
            size_t num_rx_samps = rx_stream->recv(data.buff_ptrs, samps_per_buff, md, timeout);
            //std::cout << "Receiving.." << num_rx_samps << std::endl;
            //timeout             = 0.1f; // small timeout for subsequent recv
            //std::cout << num_rx_samps << std::endl;
            if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
                std::cout << boost::format("Timeout while streaming") << std::endl;
                break;
            }
            if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
                if (overflow_message) {
                    overflow_message = false;
                    std::cerr
                        << boost::format(
                            "Got an overflow indication. Please consider the following:\n"
                            "  Your write medium must sustain a rate of %fMB/s.\n"
                            "  Dropped samples will not be written to the file.\n"
                            "  Please modify this example for your purposes.\n"
                            "  This message will not appear again.\n")
                            % (rx_rate * sizeof(std::complex<short>) / 1e6);
                }
                continue;
            }
            if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
                throw std::runtime_error(
                    str(boost::format("Receiver error %s") % md.strerror()));
            }

            num_total_samps += num_rx_samps;



            data.received_samples = num_rx_samps;
            //push buffer ptr to queue
            while(!queue_vector[threadnum].push(data));
            //std::cout << "Pushed Data" << std::endl;

        }


    // Shut down receiver
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);

    //signal writers that recieve is complete

    

}


void writeThread(std::string& file, int threadnum, int tx_chan_num, int numrepeat, DeviceSettings& deviceSettings, SignalSettings& signalSettings, size_t samps_per_buff,  std::vector<boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>>> queue_vector,std::vector<boost::atomic<bool>> done_vector){

    std::cout << "Spawning Writer Sub-thread..." << std::endl;
    uhd::set_thread_priority_safe();

    //Correctly lable output files based on run method, single TX->single RX or single TX -> All RX
    int rx_identifier;
    
    if (signalSettings.singleTXRX_loopback == true){
        rx_identifier = signalSettings.singleRX;
    }
    else{
        rx_identifier = threadnum;

    }

    const std::string this_filename = ReceiveFunctions::generateOutFilenameMultithread(file, samps_per_buff, rx_identifier, tx_chan_num, numrepeat, signalSettings, deviceSettings, threadnum);
    std::cout << "Writing to: " << this_filename << std::endl;

    unique_ptr<char[]> buf(new char[samps_per_buff]);
    ofstream output_file(this_filename, ios::out | ios::binary);
    output_file.rdbuf()->pubsetbuf(buf.get(), samps_per_buff);//Important
    recvStruct data_from_queue;

    //std::cout << "Entering Write Loop" << std::endl;
    while (!done_vector[threadnum]){
        while (queue_vector[threadnum].pop(data_from_queue)){
           // std::cout << "Task Popped..." << std::endl;
            
            //std::cout << "Writing..." << std::endl;
            output_file.write((const char*)data_from_queue.buff_ptrs[0], data_from_queue.received_samples * sizeof(std::complex<short>));
        }
    }
    while (queue_vector[threadnum].pop(data_from_queue)){
            //std::cout << "Task Popped..." << std::endl;
            
            //std::cout << "Writing..." << std::endl;
            output_file.write((const char*)data_from_queue.buff_ptrs[0], data_from_queue.received_samples * sizeof(std::complex<short>));
        }

    //TODO:: Close File if receive complete
    std::cout << "Closing File" << std::endl;
    output_file.close();
    

   
}

void testThread(boost::lockfree::spsc_queue<recvStruct> ptr_queue, int y){
    std::cout << "Test Thread" << std::endl;
}

void recvFunction(uhd::rx_streamer::sptr rx_stream,
        const std::string& cpu_format,
        const std::string& wire_format,
        const std::string& file,
        size_t samps_per_buff,
        int num_requested_samples,
        uhd::time_spec_t start_time,
        std::vector<size_t> rx_channel_nums,
        double timeout, int rx_rate, int tx_chan_num, SignalSettings& signalSettings, int numrepeat, DeviceSettings& deviceSettings, GraphSettings& graphSettings, double time_requested, int threadnum){


    boost::thread_group recv_thread_group;
    boost::thread_group write_thread_group;
    
    //boost::lockfree::spsc_queue<int, boost::lockfree::capacity<50> > spsc_queue;
    std::vector<std::vector<std::complex<short>>> buffs(100, std::vector<std::complex<short>>(samps_per_buff));

    //thread takes pointer from vector, fills with data, and then pushes to queue
    
    
    // create a vector of pointers to point to each of the channel buffers
   

   

    
    
    //recv_thread_group.create_thread(std::bind(&recvThread, rx_stream, signalSettings, num_requested_samples, time_requested, graphSettings,samps_per_buff, start_time, cpu_format,
        //wire_format, rx_channel_nums, timeout, rx_rate, threadnum));
    //recv_thread_group.create_thread(std::bind(&testThread, ptr_queue, 0));
   // write_thread_group.create_thread(std::bind(&writeThread, file, 1, threadnum, tx_chan_num, numrepeat, deviceSettings, signalSettings, samps_per_buff, threadn));

   // recv_thread_group.join_all();
    //done = true;
    //write_thread_group.join_all();
    
    std::cout << "Subthreads joined..." << std::endl;

}

 