#include "recvtofile.hpp"
#include "receivefunctions.hpp"
#include "structures.hpp"
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/utils/thread.hpp>
#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

#ifdef __linux__
#    include <unistd.h>

#endif

using namespace std;


void ReceiveControl::recvToFile(uhd::rx_streamer::sptr rx_stream,
    const std::string& cpu_format,
    const std::string& wire_format,
    const std::string& file,
    size_t samps_per_buff,
    int num_requested_samples,
    uhd::time_spec_t start_time,
    std::vector<size_t> rx_channel_nums,
    double timeout,
    int rx_rate,
    int tx_chan_num,
    SignalSettings& signalSettings,
    int numrepeat,
    DeviceSettings& deviceSettings,
    GraphSettings& graphSettings,
    double time_requested)
{
    // Recieve to file single threaded implementation.

    int num_total_samps = 0;
    // create a receive streamer
    uhd::stream_args_t stream_args(cpu_format, wire_format);
    stream_args.channels = rx_channel_nums;


    // Prepare buffers for received samples and metadata
    uhd::rx_metadata_t md;
    std::vector<std::vector<std::complex<short>>> buffs(
        rx_channel_nums.size(), std::vector<std::complex<short>>(samps_per_buff));
    // create a vector of pointers to point to each of the channel buffers
    std::vector<std::complex<short>*> buff_ptrs;
    for (size_t i = 0; i < buffs.size(); i++) {
        buff_ptrs.push_back(&buffs[i].front());
    }

    // Create one ofstream object per channel
    // (use shared_ptr because ofstream is non-copyable)
    std::vector<boost::shared_ptr<std::ofstream>> outfiles;
    for (size_t i = 0; i < buffs.size(); i++) {
        const std::string this_filename = ReceiveFunctions::generateOutFilename(file,
            i,
            tx_chan_num,
            numrepeat,
            deviceSettings.tx_freq,
            deviceSettings.folder_name);
        outfiles.push_back(boost::shared_ptr<std::ofstream>(
            new std::ofstream(this_filename.c_str(), std::ofstream::binary)));
    }
    UHD_ASSERT_THROW(outfiles.size() == buffs.size());
    UHD_ASSERT_THROW(buffs.size() == rx_channel_nums.size());
    bool overflow_message = true;

    // setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = num_requested_samples;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = start_time;
    md.has_time_spec      = true;
    md.time_spec          = start_time;

    const auto stop_time = start_time + uhd::time_spec_t(time_requested);


    rx_stream->issue_stream_cmd(stream_cmd);


    while (not stop_signal_called
           and (num_requested_samples > num_total_samps or num_requested_samples == 0)
           and (time_requested == 0.0
                or graphSettings.graph->get_mb_controller(0)
                           ->get_timekeeper(0)
                           ->get_time_now()
                       <= stop_time)) {
        size_t num_rx_samps = rx_stream->recv(buff_ptrs, samps_per_buff, md, timeout);
        std::cout << "Recv: " << num_rx_samps << std::endl;
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

        for (size_t i = 0; i < outfiles.size(); i++) {
            outfiles[i]->write(
                (const char*)buff_ptrs[i], num_rx_samps * sizeof(std::complex<short>));
        }
    }


    // Shut down receiver
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);

    // Close files
    for (size_t i = 0; i < outfiles.size(); i++) {
        outfiles[i]->close();
    }
    std::cout << "Recv Complete...(Press Ctrl+C to exit)" << std::endl;
}


void ReceiveControl::recvToMemMultithread(uhd::rx_streamer::sptr rx_stream,
    const std::string& cpu_format,
    const std::string& wire_format,
    const std::string& file,
    size_t samps_per_buff,
    int num_requested_samples,
    uhd::time_spec_t start_time,
    std::vector<size_t> rx_channel_nums,
    double timeout,
    int rx_rate,
    int tx_chan_num,
    SignalSettings& signalSettings,
    int numrepeat,
    DeviceSettings& deviceSettings,
    GraphSettings& graphSettings,
    double time_requested,
    int threadnum)
{
    // Receive to memory only, multi-threaded implementation.

    uhd::set_thread_priority_safe(0.9F);

    int num_total_samps = 0;

    uhd::stream_args_t stream_args(cpu_format, wire_format);
    stream_args.channels = rx_channel_nums;

    // Prepare buffers for received samples and metadata
    uhd::rx_metadata_t md;
    std::vector<std::vector<std::complex<short>>> buffs(
        rx_channel_nums.size(), std::vector<std::complex<short>>(samps_per_buff));
    // create a vector of pointers to point to each of the channel buffers
    std::vector<std::complex<short>*> buff_ptrs;
    for (size_t i = 0; i < buffs.size(); i++) {
        buff_ptrs.push_back(&buffs[i].front());
    }


    bool overflow_message = true;


    // setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = num_requested_samples;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = start_time;
    md.has_time_spec      = true;
    md.time_spec          = start_time;


    const auto stop_time = start_time + uhd::time_spec_t(time_requested);


    rx_stream->issue_stream_cmd(stream_cmd);

    while (not signalSettings.stop_signal_called
           and (num_requested_samples > num_total_samps or num_requested_samples == 0)
           and (time_requested == 0.0
                or graphSettings.graph->get_mb_controller(0)
                           ->get_timekeeper(0)
                           ->get_time_now()
                       <= stop_time)) {
        size_t num_rx_samps = rx_stream->recv(buff_ptrs, samps_per_buff, md, timeout);


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

        num_total_samps += num_rx_samps * rx_stream->get_num_channels();
    }
    if (stop_signal_called) {
        std::cout << stop_signal_called << std::endl;
    }


    // Shut down receiver
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);

    std::cout << "Thread: " << threadnum << " Received: " << num_total_samps
              << " samples..." << std::endl;
}


void ReceiveControl::recvToFileMultithread(uhd::rx_streamer::sptr rx_stream,
    const std::string& cpu_format,
    const std::string& wire_format,
    const std::string& file,
    size_t samps_per_buff,
    int num_requested_samples,
    uhd::time_spec_t start_time,
    std::vector<size_t> rx_channel_nums,
    double timeout,
    int rx_rate,
    int tx_chan_num,
    SignalSettings& signalSettings,
    int numrepeat,
    DeviceSettings& deviceSettings,
    GraphSettings& graphSettings,
    double time_requested,
    int threadnum)
{
    // Receive to file multi-threaded implementation.

    uhd::set_thread_priority_safe();

    int num_total_samps = 0;

    uhd::stream_args_t stream_args(cpu_format, wire_format);
    stream_args.channels = rx_channel_nums;

    unique_ptr<char[]> buf(new char[samps_per_buff]);
    // Prepare buffers for received samples and metadata
    uhd::rx_metadata_t md;
    std::vector<std::vector<std::complex<short>>> buffs(
        rx_channel_nums.size(), std::vector<std::complex<short>>(samps_per_buff));
    // create a vector of pointers to point to each of the channel buffers
    std::vector<std::complex<short>*> buff_ptrs;
    for (size_t i = 0; i < buffs.size(); i++) {
        buff_ptrs.push_back(&buffs[i].front());
    }

    // Correctly lable output files based on run method, single TX->single RX or single TX
    // -> All RX
    int rx_identifier = threadnum;

    std::vector<std::shared_ptr<std::ofstream>> outfiles;
    for (size_t i = 0; i < buffs.size(); i++) {
        // rx_identifier * 2 + i in order to get correct channel number in filename
        const std::string this_filename =
            ReceiveFunctions::generateOutFilenameMultithread(file,
                rx_identifier * 2 + i,
                tx_chan_num,
                numrepeat,
                deviceSettings.tx_freq,
                deviceSettings.folder_name,
                threadnum);
        std::ofstream* outstream =
            new std::ofstream(this_filename.c_str(), std::ofstream::binary);
        outstream->rdbuf()->pubsetbuf(buf.get(), samps_per_buff); // Important

        outfiles.push_back(std::shared_ptr<std::ofstream>(outstream));
    }
    UHD_ASSERT_THROW(outfiles.size() == buffs.size());
    UHD_ASSERT_THROW(buffs.size() == rx_channel_nums.size());


    // ofstream output_file(this_filename, ios::out | ios::binary);


    bool overflow_message = true;

    // setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = num_requested_samples;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = start_time;
    md.has_time_spec      = true;
    md.time_spec          = start_time;


    const auto stop_time = start_time + uhd::time_spec_t(time_requested);

    rx_stream->issue_stream_cmd(stream_cmd);

    while (not signalSettings.stop_signal_called
           and (num_requested_samples > num_total_samps or num_requested_samples == 0)
           and (time_requested == 0.0
                or graphSettings.graph->get_mb_controller(0)
                           ->get_timekeeper(0)
                           ->get_time_now()
                       <= stop_time)) {
        size_t num_rx_samps = rx_stream->recv(buff_ptrs, samps_per_buff, md, timeout);

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

        for (size_t i = 0; i < outfiles.size(); i++) {
            outfiles[i]->write(
                (const char*)buff_ptrs[i], num_rx_samps * sizeof(std::complex<short>));
        }

        // output_file.write((const char*)buff_ptrs[0], num_rx_samps *
        // sizeof(std::complex<short>));
    }
    if (stop_signal_called) {
        std::cout << stop_signal_called << std::endl;
    }

    // Shut down receiver
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);

    // Close files

    // output_file.close();

    // Close files
    for (size_t i = 0; i < outfiles.size(); i++) {
        outfiles[i]->close();
    }
}