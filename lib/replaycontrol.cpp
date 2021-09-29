//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "replaycontrol.hpp"
#include "blocksettings.hpp"
#include "receivefunctions.hpp"
#include "recvdata.hpp"
#include "structures.hpp"
#include "sync.hpp"
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <boost/array.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/thread.hpp>
#include <cmath>
#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>


/************************************************************************************
Import Data into USRP Replay Blocks
For now, this imports the same data into each replay block. Eventually multi-file
capability may be added.
*************************************************************************************/

int ReplayControl::importData(
    GraphSettings& graphSettings, const std::string& file, size_t& samples_to_replay)
{
    // Constants related to the Replay block
    const size_t replay_word_size = 8; // Size of words used by replay block
    const size_t sample_size      = 4; // Complex signed 16-bit is 32 bits per sample
    const size_t samples_per_word = 2; // Number of sc16 samples per word

    /************************************************************************
     * Read the data to replay
     ***********************************************************************/
    // Open the file
    std::ifstream infile(file.c_str(), std::ifstream::binary);
    if (!infile.is_open()) {
        std::cerr << "Could not open specified file" << std::endl;
        exit(0);
        return EXIT_FAILURE;
    }

    // Get the file size
    infile.seekg(0, std::ios::end);
    size_t file_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    // Calculate the number of 64-bit words and samples to replay
    size_t words_to_replay = file_size / replay_word_size;
    samples_to_replay      = file_size / sample_size;
    // std::cout << "FILESIZE:  " << file_size << std::endl;

    graphSettings.replay_buff_addr = 0;
    graphSettings.replay_buff_size = samples_to_replay * sample_size;

    // Create buffer
    std::vector<char> tx_buffer(samples_to_replay * sample_size);
    char* tx_buf_ptr = &tx_buffer[0];

    // Read file into buffer, rounded down to number of words
    infile.read(tx_buf_ptr, samples_to_replay * sample_size);
    infile.close();


    for (int i = 0; i < graphSettings.replay_ctrls.size(); i = i + 2) {
        /************************************************************************
         * Configure replay block
         ***********************************************************************/
        // Configure a buffer in the on-board memory at address 0 that's equal in
        // size to the file we want to play back (rounded down to a multiple of
        // 64-bit words). Note that it is allowed to playback a different size or
        // location from what was recorded.

        std::cout << graphSettings.replay_ctrls[i]->get_block_id() << std::endl;

        graphSettings.replay_ctrls[i]->record(
            graphSettings.replay_buff_addr, graphSettings.replay_buff_size, 0);

        // Display replay configuration
        std::cout << "Replay file size:     " << graphSettings.replay_buff_size
                  << " bytes (" << words_to_replay << " qwords, " << samples_to_replay
                  << " samples)" << std::endl
                  << "Record base address:  0x" << std::hex
                  << graphSettings.replay_ctrls[i]->get_record_offset(0) << std::dec
                  << std::endl
                  << "Record buffer size:   "
                  << graphSettings.replay_ctrls[i]->get_record_size(0) << " bytes"
                  << std::endl
                  << "Record fullness:      "
                  << graphSettings.replay_ctrls[i]->get_record_fullness(0) << " bytes"
                  << std::endl
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
                time_diff =
                    std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);
            } while (time_diff.count() < 250);
        } while (fullness);
        std::cout << "Record fullness:      "
                  << graphSettings.replay_ctrls[i]->get_record_fullness(0) << " bytes"
                  << std::endl
                  << std::endl;


        /************************************************************************
         * Send data to replay (record the data)
         ***********************************************************************/
        std::cout << "Sending data to be recorded..." << std::endl;

        uhd::tx_metadata_t tx_md;
        tx_md.start_of_burst = true;
        tx_md.end_of_burst   = true;


        size_t num_tx_samps =
            graphSettings.tx_stream_vector[i]->send(tx_buf_ptr, samples_to_replay, tx_md);

        if (num_tx_samps != samples_to_replay) {
            std::cout << "ERROR: Unable to send " << samples_to_replay << " samples"
                      << std::endl;
            return EXIT_FAILURE;
        }

        /************************************************************************
         * Wait for data to be stored in on-board memory
         ***********************************************************************/
        std::cout << "Waiting for recording to complete..." << std::endl;

        while (graphSettings.replay_ctrls[i]->get_record_fullness(0)
               < graphSettings.replay_buff_size)
            ;
        std::cout << "Record fullness:      "
                  << graphSettings.replay_ctrls[i]->get_record_fullness(0) << " bytes"
                  << std::endl
                  << std::endl;
    }
    return EXIT_SUCCESS;
}


void ReplayControl::stopReplay(GraphSettings& graphSettings)
{
    /************************************************************************
     * Issue stop command
     ***********************************************************************/
    std::cout << "Stopping replay..." << std::endl;

    for (int i_kill = 0; i_kill < graphSettings.replay_ctrls.size(); i_kill++) {
        graphSettings.replay_ctrls[i_kill]->stop(
            graphSettings.replay_chan_vector[i_kill]);

        // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


void ReplayControl::sig_int_handler(int)
{
    stop_signal_called = true;
}
