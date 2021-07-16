/*This program demonstrates reading a dat file to the replay buffer of a USRP.
It is based off of ettus research example code.  
Since there is a single replay block, only one buffer is required. 
*/
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/utils/graph_utils.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/safe_main.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <thread>

namespace po = boost::program_options;

using std::cout;
using std::endl;


static volatile bool stop_signal_called = false;

// Ctrl+C handler
void sig_int_handler(int)
{
    stop_signal_called = true;
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{

    // We use sc16 in this example, but the replay block only uses 64-bit words
    // and is not aware of the CPU or wire format.
    std::string wire_format("sc16");
    std::string cpu_format("sc16");

    // Constants related to the Replay block
    const size_t replay_word_size = 8; // Size of words used by replay block
    const size_t sample_size      = 4; // Complex signed 16-bit is 32 bits per sample
    const size_t samples_per_word = 2; // Number of sc16 samples per word

    /************************************************************************
     * Set up the program options
     ***********************************************************************/
    std::string args, tx_args, file;
    
    size_t replay_id, replay_chan;

    po::options_description desc("Allowed Options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to read binary samples from")

        ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Print help message
    if (vm.count("help")) {
        cout << "UHD/RFNoC .dat file to replay buffer example. " << desc << endl;
        cout << "This application reads data from a .dat file and stores in the replay buffer of a USRP device."
             << endl
             << endl;
        return EXIT_FAILURE;
    }



    /************************************************************************
     * Create device and block controls
     ***********************************************************************/
    std::cout << std::endl;
    std::cout << "Reading from .dat file to replay buffer with args: " << args << "..." << std::endl;
    uhd::rfnoc::rfnoc_graph::sptr graph = uhd::rfnoc::rfnoc_graph::make(args);


    // Check if the replay block exists on this device
    uhd::rfnoc::block_id_t replay_ctrl_id(0, "Replay", replay_id);
    uhd::rfnoc::replay_block_control::sptr replay_ctrl;
    if (!graph->has_block(replay_ctrl_id)) {
        cout << "Unable to find block \"" << replay_ctrl_id << "\"" << endl;
        return EXIT_FAILURE;
    }
    replay_ctrl = graph->get_block<uhd::rfnoc::replay_block_control>(replay_ctrl_id);
    std::cout << "Using replay " << replay_ctrl_id << ", channel " << replay_chan
              << std::endl;

     /************************************************************************
     * Set up streamer to Replay block
     ***********************************************************************/
    uhd::device_addr_t streamer_args;
    uhd::stream_args_t stream_args(cpu_format, wire_format);
    uhd::tx_streamer::sptr tx_stream;
    uhd::tx_metadata_t tx_md;

    streamer_args["block_id"]   = replay_ctrl->get_block_id().to_string();
    streamer_args["block_port"] = std::to_string(replay_chan);
    stream_args.args            = streamer_args;
    stream_args.channels        = {replay_chan};
    tx_stream = graph->create_tx_streamer(stream_args.channels.size(), stream_args);
    graph->connect(tx_stream, 0, replay_ctrl->get_block_id(), replay_chan);
    graph->commit();

    // Make sure that streamer SPP is a multiple of the Replay block word size
    size_t tx_spp = tx_stream->get_max_num_samps();
    if (tx_spp % samples_per_word != 0) {
        // Round SPP down to a multiple of the word size
        tx_spp = (tx_spp / samples_per_word) * samples_per_word;
        tx_stream.reset();
        streamer_args["spp"] = std::to_string(tx_spp);
        stream_args.args     = streamer_args;
        tx_stream = graph->create_tx_streamer(stream_args.channels.size(), stream_args);
        graph->connect(tx_stream, replay_chan, replay_ctrl->get_block_id(), replay_chan);
        graph->commit();
    }



    /************************************************************************
     * Read the data to replay
     ***********************************************************************/
    // Open the file
    std::ifstream infile(file.c_str(), std::ifstream::binary);
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
    size_t samples_to_replay = file_size / sample_size;

    // Create buffer
    std::vector<char> tx_buffer(samples_to_replay * sample_size);
    //reference to begining of tx_buffer
    char* tx_buf_ptr = &tx_buffer[0];

    // Read file into buffer, rounded down to number of words
    infile.read(tx_buf_ptr, samples_to_replay * sample_size);
    infile.close();


    /************************************************************************
     * Configure replay block
     ***********************************************************************/
    // Configure a buffer in the on-board memory at address 0 that's equal in
    // size to the file we want to play back (rounded down to a multiple of
    // 64-bit words). Note that it is allowed to playback a different size or
    // location from what was recorded.
    uint32_t replay_buff_addr = 0;
    uint32_t replay_buff_size = samples_to_replay * sample_size;
    replay_ctrl->record(replay_buff_addr, replay_buff_size, replay_chan);

    // Display replay configuration
    cout << "Replay file size:     " << replay_buff_size << " bytes (" << words_to_replay
         << " qwords, " << samples_to_replay << " samples)" << endl;

    cout << "Record base address:  0x" << std::hex
         << replay_ctrl->get_record_offset(replay_chan) << std::dec << endl;
    cout << "Record buffer size:   " << replay_ctrl->get_record_size(replay_chan)
         << " bytes" << endl;
    cout << "Record fullness:      " << replay_ctrl->get_record_fullness(replay_chan)
         << " bytes" << endl
         << endl;

    // Restart record buffer repeatedly until no new data appears on the Replay
    // block's input. This will flush any data that was buffered on the input.
    uint32_t fullness;
    cout << "Emptying record buffer..." << endl;
    do {
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::duration time_diff;

        replay_ctrl->record_restart(replay_chan);

        // Make sure the record buffer doesn't start to fill again
        start_time = std::chrono::system_clock::now();
        do {
            fullness = replay_ctrl->get_record_fullness(replay_chan);
            if (fullness != 0)
                break;
            time_diff = std::chrono::system_clock::now() - start_time;
            time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);
        } while (time_diff.count() < 250);
    } while (fullness);
    cout << "Record fullness:      " << replay_ctrl->get_record_fullness(replay_chan)
         << " bytes" << endl
         << endl;

    /************************************************************************
     * Send data to replay (record the data)
     ***********************************************************************/
    cout << "Sending data to be recorded..." << endl;
    tx_md.start_of_burst = true;
    tx_md.end_of_burst   = true;
    size_t num_tx_samps  = tx_stream->send(tx_buf_ptr, samples_to_replay, tx_md);

    if (num_tx_samps != samples_to_replay) {
        cout << "ERROR: Unable to send " << samples_to_replay << " samples" << endl;
        return EXIT_FAILURE;
    }

    /************************************************************************
     * Wait for data to be stored in on-board memory
     ***********************************************************************/
    cout << "Waiting for recording to complete..." << endl;
    while (replay_ctrl->get_record_fullness(replay_chan) < replay_buff_size)
        ;
    cout << "Record fullness:      " << replay_ctrl->get_record_fullness(replay_chan)
         << " bytes" << endl
         << endl;













    return EXIT_SUCCESS;
}