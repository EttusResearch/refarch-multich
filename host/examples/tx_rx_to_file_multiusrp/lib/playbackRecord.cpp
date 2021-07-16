#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/thread/thread.hpp>
#include <csignal>
#include <fstream>
#include <chrono>
#include <thread>
#include <iostream>
#include "config.hpp"
#include "waveTable.hpp"
#include "playbackRecord.hpp"

/***********************************************************************
 * Utilities
 **********************************************************************/
//! Change to filename, e.g. from usrp_samples.dat to usrp_samples.00.dat,
//  but only if multiple names are to be generated.
std::string generate_out_filename(
    const std::string& base_fn, size_t n_names, size_t this_name)
{
    if (n_names == 1) {
        return base_fn;
    }

    boost::filesystem::path base_fn_fp(base_fn);
    base_fn_fp.replace_extension(boost::filesystem::path(
        str(boost::format("%02d%s") % this_name % base_fn_fp.extension().string())));
    return base_fn_fp.string();
}

/***********************************************************************
 * Signal handlers
 **********************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
}

/***********************************************************************
 * transmit_worker function
 * A function to be used as a boost::thread_group thread for transmitting
 **********************************************************************/
void transmit_worker(std::vector<std::complex<float>> buff,
    wave_table_class wave_table,
    uhd::tx_streamer::sptr tx_streamer,
    uhd::tx_metadata_t metadata,
    size_t step,
    int num_channels,
    double txBurstReps,
    uhd::usrp::multi_usrp::sptr usrp)
{
    std::vector<std::complex<float>*> buffs(num_channels, &buff.front());
    double loop_counter = 0;
    size_t index = 0;
    // send data until the signal handler gets called
    while (not stop_signal_called && loop_counter < txBurstReps) {
        // fill the buffer with the waveform
        for (size_t n = 0; n < buff.size(); n++) {
            buff[n] = wave_table(index += step);
        }
        // send the entire contents of the buffer
        tx_streamer->send(buffs, buff.size(), metadata);
        metadata.start_of_burst = false;
        metadata.has_time_spec  = false;
        loop_counter++;
        //debug
        std::cout << boost::format("transmit loop...%s.%s") % usrp->get_time_now().get_full_secs() % usrp->get_time_now().get_frac_secs() << std::endl;
    }
    // send a mini EOB packet
    metadata.end_of_burst = true;
    tx_streamer->send("", 0, metadata);
}

/***********************************************************************
 * recv_to_file function
 **********************************************************************/
template <typename samp_type>
void recv_to_file(uhd::usrp::multi_usrp::sptr usrp,
    const std::string& cpu_format,
    const std::string& wire_format,
    const std::string& file,
    size_t samps_per_buff,
    int num_requested_samples,
    uhd::time_spec_t start_time,
    std::vector<size_t> rxChannelNums,
    double timeout)
{
    int num_total_samps = 0;
    // create a receive streamer
    uhd::stream_args_t stream_args(cpu_format, wire_format);
    stream_args.channels             = rxChannelNums;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
    // Prepare buffers for received samples and metadata
    uhd::rx_metadata_t md;
    std::vector<std::vector<samp_type>> buffs(
        rxChannelNums.size(), std::vector<samp_type>(samps_per_buff));
    // create a vector of pointers to point to each of the channel buffers
    std::vector<samp_type*> buff_ptrs;
    for (size_t i = 0; i < buffs.size(); i++) {
        buff_ptrs.push_back(&buffs[i].front());
    }
    // Create one ofstream object per channel
    // (use shared_ptr because ofstream is non-copyable)
    std::vector<boost::shared_ptr<std::ofstream>> outfiles;
    for (size_t i = 0; i < buffs.size(); i++) {
        const std::string this_filename = generate_out_filename(file, buffs.size(), i);
        outfiles.push_back(boost::shared_ptr<std::ofstream>(
            new std::ofstream(this_filename.c_str(), std::ofstream::binary)));
    }
    UHD_ASSERT_THROW(outfiles.size() == buffs.size());
    UHD_ASSERT_THROW(buffs.size() == rxChannelNums.size());
    bool overflow_message = true;
    //double timeout = settling_time + 0.1f; // expected settling time + padding for first recv
    // setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                    ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                    : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = num_requested_samples;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = start_time;
    std::cout << boost::format("Start time for recv to file...%s.%s") % int(start_time.get_full_secs()) % start_time.get_frac_secs() << std::endl;
    std::cout << boost::format("get_time_now in recv function...%s.%s") % int(usrp->get_time_now().get_full_secs()) % usrp->get_time_now().get_frac_secs() << std::endl;
    rx_stream->issue_stream_cmd(stream_cmd);
    while (not stop_signal_called
        and (num_requested_samples > num_total_samps or num_requested_samples == 0)) {
        std::cout << boost::format("get_time_now in recv funciton loop...%s.%s") % int(usrp->get_time_now().get_full_secs()) % usrp->get_time_now().get_frac_secs() << std::endl;
        size_t num_rx_samps = rx_stream->recv(buff_ptrs, samps_per_buff, md, timeout);
        //timeout             = 0.1f; // small timeout for subsequent recv
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
                        % (usrp->get_rx_rate() * sizeof(samp_type) / 1e6);
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
                (const char*)buff_ptrs[i], num_rx_samps * sizeof(samp_type));
        }
    }
    // Shut down receiver
    // Shut down receiver
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);
    // Close files
    for (size_t i = 0; i < outfiles.size(); i++) {
        outfiles[i]->close();
    }
}

void PlaybackRecord::startStreamer()
{
    // pre-compute the waveform values
    const wave_table_class wave_table(waveType, ampl);
    const size_t step =
        boost::math::iround(waveFreq / usrpSession->get_tx_rate() * wave_table_len);
    size_t index = 0;

    // allocate a buffer which we re-use for each channel
    if (spb == 0)
        spb = txStream->get_max_num_samps() * 10;
    std::vector<std::complex<float>> buff(spb);
    int num_channels = txChannelNums.size();

     //Signal handlers
    if (totalNumSamps == 0) {
        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }

    for (int i = 0; i < runs; i++)
    {
        //update file name for each run
        std::string file_run = "run" + std::to_string(i) + "_";
        file_run.append(dataFile);
        std::cout << boost::format("Starting run %s of %s and saving data to file: %s") % i % runs % file_run
                << std::endl;

        // TX/RX shared start time
        uhd::time_spec_t tx_rx_shared_start_time = uhd::time_spec_t(usrpSession->get_time_now() + uhd::time_spec_t(settling));
        std::cout << boost::format("Shared start time in main...%s.%s") % tx_rx_shared_start_time.get_full_secs() % tx_rx_shared_start_time.get_frac_secs() << std::endl;
        std::cout << boost::format("get_time_now in main...%s.%s") % usrpSession->get_time_now().get_full_secs() % usrpSession->get_time_now().get_frac_secs() << std::endl;

        // setup the metadata flags
        uhd::tx_metadata_t md;
        md.start_of_burst = true;
        md.end_of_burst   = false;
        md.has_time_spec  = true;
        md.time_spec = tx_rx_shared_start_time; // start at same time as RX
        
        stop_signal_called = false;
        // start transmit worker thread
        boost::thread_group transmit_thread;
        transmit_thread.create_thread(boost::bind(
            &transmit_worker, buff, wave_table, txStream, md, step, num_channels, txBurstReps, usrpSession));

        // recv to file
        if (fileType == "double")
            recv_to_file<std::complex<double>>(
                usrpSession, "fc64", otw, file_run, spb, totalNumSamps, tx_rx_shared_start_time, rxChannelNums, rxTimeout); //rx_usrp
        else if (fileType == "float")
            recv_to_file<std::complex<float>>(
                usrpSession, "fc32", otw, file_run, spb, totalNumSamps, tx_rx_shared_start_time, rxChannelNums, rxTimeout); //rx_usrp
        else if (fileType == "short")
            recv_to_file<std::complex<short>>(
                usrpSession, "sc16", otw, file_run, spb, totalNumSamps, tx_rx_shared_start_time, rxChannelNums, rxTimeout); //rx_usrp
        else {
            // clean up transmit worker
            stop_signal_called = true;
            transmit_thread.join_all();
            throw std::runtime_error("Unknown type " + fileType);
        }

        //debug
        std::cout << boost::format("after recv loop...%s.%s") % usrpSession->get_time_now().get_full_secs() % usrpSession->get_time_now().get_frac_secs() << std::endl;

        // clean up transmit worker
        stop_signal_called = true;
        transmit_thread.join_all();

        //wait a second before looping
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    } //for loop for runs
}