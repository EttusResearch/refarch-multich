//
// Copyright 2021-2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "RefArch.hpp"
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/utils/thread.hpp>
#include <stdio.h>
#include <boost/circular_buffer.hpp>
#include <csignal>
#include <fstream>

#if HAS_STD_FILESYSTEM
#    if HAS_STD_FILESYSTEM_EXPERIMENTAL
#        include <experimental/filesystem>
#    else
#        include <filesystem>
#    endif
#else
#    include <boost/filesystem/operations.hpp> // for create_directories, exists
#    include <boost/filesystem/path.hpp> // for path, operator<<
#    include <boost/filesystem/path_traits.hpp> // for filesystem
#endif

#if HAS_STD_FILESYSTEM
#    if HAS_STD_FILESYSTEM_EXPERIMENTAL
namespace RA_filesystem = std::experimental::filesystem;
#    else
namespace RA_filesystem = std::filesystem;
#    endif
#else
namespace RA_filesystem = boost::filesystem;
#endif

bool RefArch::RA_stop_signal_called = false;


void RefArch::parseConfig()
{
    addProgramOptions();
    addAdditionalOptions(); // Overloaded by User
    storeProgramOptions();
    addAddressToArgs();
}


// structures
void RefArch::addProgramOptions()
{
    namespace po = boost::program_options;
    // clang-format off
        //TODO: Verify we are still using the comments for each value in
        //or if we can delete and push explanation to top of each structure

        RA_desc.add_options()
        ("cfgFile",
                po::value<std::string>(&RA_cfgFile)->required(), 
                "relative path to configuration file")
        ("args",
            po::value<std::string>(&RA_args)->default_value(""), 
            "uhd transmit device address args")
        ("tx-rate",
            po::value<double>(&RA_tx_rate)->default_value(200e6), 
            "rate of transmit outgoing samples")
        ("rx-rate",
            po::value<double>(&RA_rx_rate)->default_value(25e6), 
            "rate of receive incoming samples")
        ("tx-gain",
            po::value<double>(&RA_tx_gain)->default_value(0), 
            "gain for the transmit RF chain")
        ("rx-gain",
            po::value<double>(&RA_rx_gain)->default_value(0), 
            "gain for the receive RF chain")
        ("ref",
            po::value<std::string>(&RA_ref)->default_value("external"), 
            "clock reference (internal, external, mimo)")
        ("tx-freq",
            po::value<double>(&RA_tx_freq)->default_value(2000e6), 
            "transmit RF center frequency in Hz")
        ("rx-freq",
            po::value<double>(&RA_rx_freq)->default_value(2000e6), 
            "receive RF center frequency in Hz")
        ("address",
            po::value<std::vector<std::string>>(&RA_address), 
            "uhd transmit device address args")
        ("tx-ant",
            po::value<std::string>(&RA_tx_ant)->default_value("TX/RX"), 
            "transmit antenna selection")
        ("rx-ant",
            po::value<std::string>(&RA_rx_ant)->default_value("RX2"), 
            "receive antenna selection")
        ("streamargs", po::value<std::string>(&RA_streamargs)
        ->default_value(""),"stream args")
        ("tx-bw",
            po::value<double>(&RA_tx_bw)->default_value(0), 
            "analog transmit filter bandwidth in Hz")
        ("rx-bw",
            po::value<double>(&RA_rx_bw)->default_value(0), 
            "analog receive filter bandwidth in Hz")
        ("lo",
            po::value<std::vector<std::string>>(&RA_lo), 
            "device LO settings")
        ("rx-file",
            po::value<std::string>(&RA_rx_file)->default_value("test.dat"), 
            "name of the file to write binary samples to")
        ("rx-file-location",
             po::value<std::vector<std::string>>(&RA_rx_file_location))
        ("rx-file-channels",
            po::value<std::vector<std::string>>(&RA_rx_file_channels))
        ("otw", 
            po::value<std::string>(&RA_otw)->default_value("sc16"), 
            "specify the over-the-wire sample mode")
        ("type", 
            po::value<std::string>(&RA_type)->default_value("short"), 
            "sample type in file: double, float, or short")
        ("spb", 
            po::value<size_t>(&RA_spb)->default_value(0), 
            "samples per buffer, 0 for default")
        ("rx_timeout", 
            po::value<double>(&RA_rx_timeout)->default_value(double(3)), 
            "number of seconds before rx streamer times out")
        ("format", 
            po::value<std::string>(&RA_format)->default_value("sc16"), 
            "File sample format: sc16, fc32, or fc64")
        ("file", 
            po::value<std::string>(&RA_file)->default_value("usrp_samples.dat"), 
            "name of the file to transmit")
        ("nsamps", 
            po::value<size_t>(&RA_nsamps)->default_value(16000), 
            "number of samples to play (0 for infinite)")
        ("replay_time",
            po::value<double>(&RA_delay_start_time)->default_value(2.0), 
            "Replay Block Time Delay (seconds)")
        ("singleTX",
            po::value<int>(&RA_singleTX)->default_value(0), 
            "TX Channel)")
        ("TXAllChan",
            po::value<bool>(&RA_TX_All_Chan)->default_value(false), 
            "Transmit on all TX Channels")
        ("time_requested", 
            po::value<double>(&RA_time_requested)->default_value(0.0), 
            "Single Loopback Continuous Time Limit (s).")
        ;
    // clang-format on
}
void RefArch::addAddressToArgs()
{
    RA_argsWithAddress = "" + RA_args;
    for (const auto& addr : RA_address) {
        RA_argsWithAddress += ", " + addr;
    }
    RA_args = RA_argsWithAddress;
}
void RefArch::storeProgramOptions()
{
    namespace po = boost::program_options;
    // store program options from cmd line
    po::store(po::command_line_parser(RA_argc, RA_argv).options(RA_desc).run(), RA_vm);
    // store program options from config file
    if (RA_vm.count("cfgFile")) {
        // Have to use a special way of receiving cfgFile because we haven't run notify
        std::cout << "Load cfg_file: " << RA_vm["cfgFile"].as<std::string>() << std::endl;
        // standard streams don't accept a standard string, so pass the string using
        // c_str()
        auto parsed_options = po::parse_config_file(
            RA_vm["cfgFile"].as<std::string>().c_str(), RA_desc, true);
        po::store(parsed_options, RA_vm);
        for (const auto& o : parsed_options.options) {
            if (RA_vm.find(o.string_key) == RA_vm.end()) {
                // an unknown option
                UHD_LOG_WARNING("ConfigFile Option", "Ignoring Key: " << o.string_key);
            }
        }
    }
    po::notify(RA_vm);
}
void RefArch::setSources()
{
    // Set clock reference
    std::cout << "Locking motherboard reference/time sources..." << std::endl;
    // Lock mboard clocks
    for (size_t i = 0; i < RA_graph->get_num_mboards(); ++i) {
        RA_graph->get_mb_controller(i)->set_clock_source(RA_ref);
        RA_graph->get_mb_controller(i)->set_time_source(RA_ref);
    }
}
int RefArch::syncAllDevices()
{
    // Synchronize Devices
    bool sync_result;
    const uhd::time_spec_t syncTime = 0.0;
    sync_result                     = RA_graph->synchronize_devices(syncTime, true);
    // wait for one sec to ensure rising edge is found
    // TODO: Double check this
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Synchronized" << std::endl;
    if (sync_result != true) {
        std::cout << "Unable to Synchronize Devices " << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
void RefArch::killLOs()
{
    std::cout << "Shutting Down LOs" << std::endl;
    size_t device = 0;
    while (device < RA_lo.size()) {
        if (RA_lo[device] == "source" or RA_lo[device] == "distributor") {
            RA_graph->get_tree()
                ->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device
                                   % "/Radio#0/dboard/tx_frontends/0/los/lo1/"
                                     "lo_distribution/LO_OUT_0/export"))
                .set(false);
            RA_graph->get_tree()
                ->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device
                                   % "/Radio#0/dboard/tx_frontends/0/los/lo1/"
                                     "lo_distribution/LO_OUT_1/export"))
                .set(false);
            RA_graph->get_tree()
                ->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device
                                   % "/Radio#0/dboard/tx_frontends/0/los/lo1/"
                                     "lo_distribution/LO_OUT_2/export"))
                .set(false);
            RA_graph->get_tree()
                ->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device
                                   % "/Radio#0/dboard/tx_frontends/0/los/lo1/"
                                     "lo_distribution/LO_OUT_3/export"))
                .set(false);

            RA_graph->get_tree()
                ->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device
                                   % "/Radio#0/dboard/rx_frontends/0/los/lo1/"
                                     "lo_distribution/LO_OUT_0/export"))
                .set(false);
            RA_graph->get_tree()
                ->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device
                                   % "/Radio#0/dboard/rx_frontends/0/los/lo1/"
                                     "lo_distribution/LO_OUT_1/export"))
                .set(false);
            RA_graph->get_tree()
                ->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device
                                   % "/Radio#0/dboard/rx_frontends/0/los/lo1/"
                                     "lo_distribution/LO_OUT_2/export"))
                .set(false);
            RA_graph->get_tree()
                ->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device
                                   % "/Radio#0/dboard/rx_frontends/0/los/lo1/"
                                     "lo_distribution/LO_OUT_3/export"))
                .set(false);
        }
        device++;
    }
    std::cout << "Shutting Down LOs: Done!" << std::endl;
}
void RefArch::setLOsfromConfig()
{
    // Set LOs per config from config file
    int device = 0;
    for (size_t device = 0; device < RA_lo.size(); device++) {
        if (RA_lo[device] == "source") {
            RefArch::setSource(device);
        } else if (RA_lo[device] == "terminal") {
            RefArch::setTerminal(device);
        } else if (RA_lo[device] == "distributor") {
            RefArch::setDistributor(device);
        }
    }
}
void RefArch::setSource(int device)
{
    std::cout << "Setting Device# " << device << " Radio# " << device * 2 << ", Radio# "
              << device * 2 + 1 << " to: "
              << "source" << std::endl;
    // Set Device to System LO Source
    // No difference between RX and TX LOs, just used RX.
    // TODO: Hardcoded number of channels per device.
    RA_radio_ctrls[device * 2]->set_tx_lo_export_enabled(false, "lo1", 0);
    RA_radio_ctrls[device * 2 + 1]->set_tx_lo_export_enabled(false, "lo1", 0);

    RA_radio_ctrls[device * 2]->set_rx_lo_export_enabled(true, "lo1", 0);
    RA_radio_ctrls[device * 2 + 1]->set_rx_lo_export_enabled(true, "lo1", 0);

    // TODO: this did not clean up nice. Reformat.
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export"))
        .set(true);

    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export"))
        .set(true);

    RA_radio_ctrls[device * 2]->set_tx_lo_source("external", "lo1", 0);
    RA_radio_ctrls[device * 2 + 1]->set_tx_lo_source("external", "lo1", 0);

    RA_radio_ctrls[device * 2]->set_rx_lo_source("external", "lo1", 0);
    RA_radio_ctrls[device * 2 + 1]->set_rx_lo_source("external", "lo1", 0);
}
void RefArch::setTerminal(int device)
{
    // TODO: Hardcoded number of channels per device.
    std::cout << "Setting Device# " << device << " Radio# " << device * 2 << ", Radio# "
              << device * 2 + 1 << " to: "
              << "terminal" << std::endl;
    RA_radio_ctrls[device * 2]->set_tx_lo_source("external", "lo1", 0);
    RA_radio_ctrls[device * 2 + 1]->set_tx_lo_source("external", "lo1", 0);

    RA_radio_ctrls[device * 2]->set_rx_lo_source("external", "lo1", 0);
    RA_radio_ctrls[device * 2 + 1]->set_rx_lo_source("external", "lo1", 0);
}
void RefArch::setDistributor(int device)
{
    // TODO: Hardcoded number of channels per device.
    std::cout << "Setting Device# " << device << " Radio# " << device * 2 << ", Radio# "
              << device * 2 + 1 << " to: "
              << "distributor" << std::endl;

    RA_radio_ctrls[device * 2]->set_tx_lo_source("external", "lo1", 0);
    RA_radio_ctrls[device * 2 + 1]->set_tx_lo_source("external", "lo1", 0);

    RA_radio_ctrls[device * 2]->set_rx_lo_source("external", "lo1", 0);
    RA_radio_ctrls[device * 2 + 1]->set_rx_lo_source("external", "lo1", 0);

    RA_radio_ctrls[device * 2]->set_tx_lo_export_enabled(false, "lo1", 0);
    RA_radio_ctrls[device * 2 + 1]->set_tx_lo_export_enabled(false, "lo1", 0);

    RA_radio_ctrls[device * 2]->set_rx_lo_export_enabled(false, "lo1", 0);
    RA_radio_ctrls[device * 2 + 1]->set_rx_lo_export_enabled(false, "lo1", 0);

    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export"))
        .set(true);

    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export"))
        .set(true);
    RA_graph->get_tree()
        ->access<bool>(str(
            boost::format("%s%d%s") % "blocks/" % device
            % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export"))
        .set(true);
}
void RefArch::checkRXSensorLock()
{
    // Check Locked RX Sensors
    std::vector<std::string> rx_sensor_names;
    for (auto& rctrl : RA_radio_ctrls) {
        rx_sensor_names = rctrl->get_rx_sensor_names(0);
        for (auto& name : rx_sensor_names) {
            uhd::sensor_value_t rx_sensor_value = rctrl->get_rx_sensor(name, 0);
            std::cout << "Checking RX LO Lock: " << rx_sensor_value.to_pp_string()
                      << std::endl;
            // TODO: change to !rx_sensor_value.to_bool()
            while (rx_sensor_value.to_pp_string() != "all_los: locked") {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "RX LO LOCKED" << std::endl;
        }
    }
}
void RefArch::checkTXSensorLock()
{
    // Check Locked TX Sensors
    std::vector<std::string> tx_sensor_names;
    for (auto& rctrl : RA_radio_ctrls) {
        tx_sensor_names = rctrl->get_tx_sensor_names(0);
        for (auto& name : tx_sensor_names) {
            uhd::sensor_value_t tx_sensor_value = rctrl->get_tx_sensor(name, 0);
            std::cout << "Checking TX LO Lock: " << tx_sensor_value.to_pp_string()
                      << std::endl;
            // TODO: change to !tx_sensor_value.to_bool()
            while (tx_sensor_value.to_pp_string() != "all_los: locked") {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "TX LO LOCKED" << std::endl;
        }
    }
}
void RefArch::updateDelayedStartTime()
{
    // This provides a common timebase to synchronize RX and TX threads.
    uhd::time_spec_t now =
        RA_graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now();
    RA_start_time = uhd::time_spec_t(now + RA_delay_start_time);
}
// replaycontrol
int RefArch::importData()
{
    // Constants related to the Replay block
    const size_t replay_word_size = 8; // Size of words used by replay block
    const size_t sample_size      = 4; // Complex signed 16-bit is 32 bits per sample
    const size_t samples_per_word = 2; // Number of sc16 samples per word
    /************************************************************************
     * Read the data to replay
     ***********************************************************************/
    // Open the file
    std::ifstream infile(RA_file.c_str(), std::ifstream::binary);
    if (!infile.is_open()) {
        std::cerr << "Could not open Replay file. Try using absolute path:" << std::endl
                  << RA_file << std::endl;
        exit(0);
        return EXIT_FAILURE;
    }
    // Get the file size
    infile.seekg(0, std::ios::end);
    size_t file_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    // Calculate the number of 64-bit words and samples to replay
    size_t words_to_replay = file_size / replay_word_size;
    RA_samples_to_replay   = file_size / sample_size;
    RA_replay_buff_addr    = 0;
    RA_replay_buff_size    = RA_samples_to_replay * sample_size;

    // Create buffer
    std::vector<char> tx_buffer(RA_samples_to_replay * sample_size);
    char* tx_buf_ptr = &tx_buffer[0];

    // Read file into buffer, rounded down to number of words
    infile.read(tx_buf_ptr, RA_samples_to_replay * sample_size);
    infile.close();
    for (size_t i = 0; i < RA_replay_ctrls.size(); i = i + 2) {
        /************************************************************************
         * Configure replay block
         ***********************************************************************/
        // Configure a buffer in the on-board memory at address 0 that's equal in
        // size to the file we want to play back (rounded down to a multiple of
        // 64-bit words). Note that it is allowed to playback a different size or
        // location from what was recorded.
        std::cout << RA_replay_ctrls[i]->get_block_id() << std::endl;
        RA_replay_ctrls[i]->record(RA_replay_buff_addr, RA_replay_buff_size, 0);
        // Display replay configuration
        std::cout << "Replay file size:     " << RA_replay_buff_size << " bytes ("
                  << words_to_replay << " qwords, " << RA_samples_to_replay << " samples)"
                  << std::endl
                  << "Record base address:  0x" << std::hex
                  << RA_replay_ctrls[i]->get_record_offset(0) << std::dec << std::endl
                  << "Record buffer size:   " << RA_replay_ctrls[i]->get_record_size(0)
                  << " bytes" << std::endl
                  << "Record fullness:      "
                  << RA_replay_ctrls[i]->get_record_fullness(0) << " bytes" << std::endl
                  << std::endl;
        // Restart record buffer repeatedly until no new data appears on the Replay
        // block's input. This will flush any data that was buffered on the input.
        uint32_t fullness;
        std::cout << "Emptying record buffer..." << std::endl;
        do {
            std::chrono::system_clock::time_point replay_start_time;
            std::chrono::system_clock::duration time_diff;
            RA_replay_ctrls[i]->record_restart(0);
            // Make sure the record buffer doesn't start to fill again
            replay_start_time = std::chrono::system_clock::now();
            do {
                fullness = RA_replay_ctrls[i]->get_record_fullness(0);
                if (fullness != 0) {
                    std::cout << "BREAK" << std::endl;
                    break;
                }
                time_diff = std::chrono::system_clock::now() - replay_start_time;
                time_diff =
                    std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);
            } while (time_diff.count() < 250);
        } while (fullness);
        std::cout << "Record fullness:      "
                  << RA_replay_ctrls[i]->get_record_fullness(0) << " bytes" << std::endl
                  << std::endl;

        /************************************************************************
         * Send data to replay (record the data)
         ***********************************************************************/
        std::cout << "Sending data to be recorded..." << std::endl;
        uhd::tx_metadata_t tx_md;
        tx_md.start_of_burst = true;
        tx_md.end_of_burst   = true;
        size_t num_tx_samps =
            RA_tx_stream_vector[i]->send(tx_buf_ptr, RA_samples_to_replay, tx_md);
        if (num_tx_samps != RA_samples_to_replay) {
            std::cout << "ERROR: Unable to send " << RA_samples_to_replay << " samples"
                      << std::endl;
            return EXIT_FAILURE;
        }

        /************************************************************************
         * Wait for data to be stored in on-board memory
         ***********************************************************************/
        std::cout << "Waiting for recording to complete..." << std::endl;
        while (RA_replay_ctrls[i]->get_record_fullness(0) < RA_replay_buff_size)
            ;
        std::cout << "Record fullness:      "
                  << RA_replay_ctrls[i]->get_record_fullness(0) << " bytes" << std::endl
                  << std::endl;
    }
    return EXIT_SUCCESS;
}
void RefArch::stopReplay()
{
    /************************************************************************
     * Issue stop command
     ***********************************************************************/
    std::cout << "Stopping replay..." << std::endl;
    for (size_t i_kill = 0; i_kill < RA_replay_ctrls.size(); i_kill++) {
        RA_replay_ctrls[i_kill]->stop(RA_replay_chan_vector[i_kill]);
    }
}
void RefArch::sigIntHandler(int)
{
    RA_stop_signal_called = true;
}
// receivefunctions
std::map<int, std::string> RefArch::getStreamerFileLocation(
    const std::vector<std::string>& rx_file_streamers,
    const std::vector<std::string>& rx_file_location)
{
    std::map<int, std::string> usrpFileMap;
    for (size_t i = 0; i < rx_file_streamers.size(); i++) {
        size_t pos = 0;
        int n;
        std::string usrpStringArray = rx_file_streamers[i];
        while (1) {
            try {
                int n           = std::stoi(usrpStringArray, &pos);
                usrpFileMap[n]  = rx_file_location[i];
                usrpStringArray = usrpStringArray.substr(pos);
            } catch (...) {
                break;
            }
        }
    }
    return usrpFileMap;
}
std::string RefArch::generateRxFilename(const std::string& base_fn,
    const size_t rx_chan_num,
    const int tx_chan_num,
    const int run_num,
    const double tx_freq,
    const std::string& folder_name,
    const std::vector<std::string>& rx_streamer_string,
    const std::vector<std::string>& rx_file_location)
{
    // Generates filenames for multithreaded implementation.
    if (rx_file_location.size() != rx_streamer_string.size()) {
        throw std::runtime_error("The number of Streamers file locations must match the "
                                 "number of Folder Names");
    }
    std::map<int, std::string> streamer_files =
        getStreamerFileLocation(rx_streamer_string, rx_file_location);
    try {
        std::string cw_folder =
            "CW_" + std::to_string(tx_freq * 1e-9) + "_GHz_" + folder_name;
        RA_filesystem::create_directory(RA_filesystem::path(
            str(boost::format("%s%s") % streamer_files.at(rx_chan_num) % cw_folder)));
        RA_filesystem::path base_fn_fp(
            streamer_files.at(rx_chan_num) + cw_folder + "/" + base_fn);
        base_fn_fp.replace_extension(RA_filesystem::path(
            str(boost::format("%s%02d%s%02d%s%02d%s%02d%s") % "tx_" % tx_chan_num % "_rx_"
                % rx_chan_num % "_run_" % run_num % "_cw_" % tx_freq
                % base_fn_fp.extension().string())));
        return base_fn_fp.string();
    } catch (const std::out_of_range& oor) {
        throw uhd::runtime_error(
            "One or more file locations were not specified for initialized channel.");
    }
}
// graphassembly
void RefArch::buildGraph()
{
    /************************************************************************
     * Create device and block controls
     ***********************************************************************/
    // If multiple USRPs are used, they are linked into a single RFNoc graph here.
    std::cout << std::endl;
    std::cout << "Creating the RFNoC graph with args: " << RA_args << "..." << std::endl;
    RA_graph = uhd::rfnoc::rfnoc_graph::make(RA_args);
}
void RefArch::buildRadios()
{
    /************************************************************************
     * Seek radio blocks on each USRP and assemble a vector of radio
     * controllers.
     ***********************************************************************/
    std::cout << "Building Radios..." << std::endl;
    // Make radio controllers for multiple channels/devices
    RA_radio_block_list = RA_graph->find_blocks("Radio");
    // Iterate over each radio block found on each device
    for (auto& elem : RA_radio_block_list) {
        // Create a vector of radio control objects for controlling the radio blocks
        RA_radio_ctrls.push_back(RA_graph->get_block<uhd::rfnoc::radio_control>(elem));
        std::cout << "Using radio " << elem << std::endl;
    }
    // Sort the vectors
    sort(RA_radio_ctrls.begin(), RA_radio_ctrls.end());
    sort(RA_radio_block_list.begin(), RA_radio_block_list.end());
}
void RefArch::buildDDCDUC()
{
    /*************************************************************************
     * Seek DDCs & DUCs on each USRP and assemble a vector of DDC & DUC controllers.
     ************************************************************************/
    std::cout << "Building DDC/DUCs.." << std::endl;
    std::vector<std::string> ddc_sources;
    std::vector<std::string> duc_destinations;
    // Enumerate blocks in the chain
    auto edges = RA_graph->enumerate_static_connections();

    // vector of string identifiers for DDC destinations
    for (auto& rctrl : RA_radio_ctrls) {
        ddc_sources.push_back(rctrl->get_block_id());
        duc_destinations.push_back(rctrl->get_block_id());
    }
    sort(ddc_sources.begin(), ddc_sources.end());
    size_t ddc_source_port = 0;
    size_t duc_dst_port    = 0;
    auto chain             = std::vector<uhd::rfnoc::graph_edge_t>();
    RA_ddc_chan            = 0;
    RA_duc_chan            = 0;

    // Find DDCs connected to the radios
    // There is a single channel (src_port) included on the stock DDCs & DUCs on the N3xx
    // XG image, 0.
    // Look at each connection in the RFNoc graph
    for (auto& edge : edges) {
        // Loop over the radio blocks and look for connected DDCs
        for (auto& src_block : ddc_sources) {
            if (edge.src_blockid == src_block && edge.src_port == ddc_source_port) {
                auto blockid = uhd::rfnoc::block_id_t(edge.dst_blockid);
                if (blockid.match("DDC")) {
                    // When a DUC is found, assemble a vector of DDC controllers.
                    RA_ddc_ctrls.push_back(
                        RA_graph->get_block<uhd::rfnoc::ddc_block_control>(blockid));
                }
            }
        }
        for (auto& dst_block : duc_destinations) {
            if (edge.dst_blockid == dst_block && edge.dst_port == duc_dst_port) {
                auto blockid = uhd::rfnoc::block_id_t(edge.src_blockid);
                if (blockid.match("DUC")) {
                    // When a DUC is found, assemble a vector of DUC controllers.
                    RA_duc_ctrls.push_back(
                        RA_graph->get_block<uhd::rfnoc::duc_block_control>(blockid));
                }
            }
        }
        chain.push_back(edge);
    }
    sort(RA_ddc_ctrls.begin(), RA_ddc_ctrls.end());
    sort(RA_duc_ctrls.begin(), RA_duc_ctrls.end());
    // For Display Purposes
    for (auto& dctrl : RA_ddc_ctrls) {
        std::cout << "Using DDC " << dctrl->get_block_id() << ", Channel " << RA_ddc_chan
                  << std::endl;
    }
    // For Display Purposes
    for (auto& dctrl : RA_duc_ctrls) {
        std::cout << "Using DUC " << dctrl->get_block_id() << ", Channel " << RA_duc_chan
                  << std::endl;
    }
}
void RefArch::buildReplay()
{
    /****************************************************************************
     * Seek Replay blocks on each USRP and assemble a vector of Replay Block Controllers
     ***************************************************************************/
    // Check what replay blocks exist on the device(s)
    RA_replay_block_list = RA_graph->find_blocks("Replay");
    for (auto& replay_id : RA_replay_block_list) {
        // The double entry is intended. There are two channels on each replay block.
        RA_replay_ctrls.push_back(
            RA_graph->get_block<uhd::rfnoc::replay_block_control>(replay_id));
        RA_replay_ctrls.push_back(
            RA_graph->get_block<uhd::rfnoc::replay_block_control>(replay_id));
    }
    sort(RA_replay_ctrls.begin(), RA_replay_ctrls.end());
    // Each replay block has two channels, 0 and 1.
    // Vector of replay channels.
    for (size_t i_chan = 0; i_chan < RA_replay_ctrls.size() * 2; i_chan++) {
        if (i_chan % 2 == 0) {
            RA_replay_chan_vector.push_back(0);
        } else {
            RA_replay_chan_vector.push_back(1);
        }
    }
    // For Display Purposes
    int count_replay_chan = 0;
    for (auto& rctrl : RA_replay_ctrls) {
        std::cout << "Using Replay " << rctrl->get_block_id() << ", Channel "
                  << RA_replay_chan_vector[count_replay_chan] << std::endl;
        count_replay_chan++;
    }
}
void RefArch::commitGraph()
{
    UHD_LOG_INFO("CogRF", "Committing graph...");
    RA_graph->commit();
    UHD_LOG_INFO("CogRF", "Commit complete.");
}
void RefArch::connectGraphMultithread()
{
    // This is the function that connects the graph for the multithreaded implementation
    // streaming from Replay Block. The difference is that each channel gets its own RX
    // streamer.
    UHD_LOG_INFO("CogRF", "Connecting graph...");
    // Connect Graph
    for (size_t i = 0; i < RA_radio_ctrls.size(); i++) {
        // connect radios to ddc
        RA_graph->connect(RA_radio_block_list[i], 0, RA_ddc_ctrls[i]->get_block_id(), 0);
        std::cout << "Connected " << RA_radio_block_list[i] << " to "
                  << RA_ddc_ctrls[i]->get_block_id() << std::endl;
    }
    // Each streamer has two channels, 0 and 1.
    // Vector of streamer channels.
    for (size_t i_chan = 0; i_chan < RA_rx_stream_vector.size() * 2; i_chan++) {
        if (i_chan % 2 == 0) {
            RA_rx_stream_chan_vector.push_back(0);
        } else {
            RA_rx_stream_chan_vector.push_back(1);
        }
    }
    for (size_t j = 0; j < RA_ddc_ctrls.size(); j++) {
        // Connect DDC to streamers
        RA_graph->connect(RA_ddc_ctrls[j]->get_block_id(),
            0,
            RA_rx_stream_vector[j],
            RA_rx_stream_chan_vector[j]);
        std::cout << "Connected " << RA_ddc_ctrls[j]->get_block_id() << " to "
                  << RA_rx_stream_vector[j] << " Port " << RA_rx_stream_chan_vector[j]
                  << std::endl;
    }
    int pos2 = 0;
    if (RA_duc_ctrls.size() > 0) {
        for (auto& rctrl : RA_radio_ctrls) {
            RA_graph->connect(RA_duc_ctrls[pos2]->get_block_id(),
                RA_duc_chan,
                rctrl->get_block_id(),
                0);
            std::cout << "Connected " << RA_duc_ctrls[pos2]->get_block_id() << " port "
                      << RA_duc_chan << " to radio " << rctrl->get_block_id() << " port "
                      << 0 << std::endl;
            pos2++;
        }
        for (size_t i_r2d = 0; i_r2d < RA_duc_ctrls.size(); i_r2d++) {
            RA_graph->connect(RA_replay_ctrls[i_r2d]->get_block_id(),
                RA_replay_chan_vector[i_r2d],
                RA_duc_ctrls[i_r2d]->get_block_id(),
                RA_duc_chan);
            std::cout << "Connected " << RA_replay_ctrls[i_r2d]->get_block_id()
                      << " port " << RA_replay_chan_vector[i_r2d] << " to DUC "
                      << RA_duc_ctrls[i_r2d]->get_block_id() << " port " << RA_duc_chan
                      << std::endl;
        }
    } else {
        // For the case where the replay block is connected directly to the radio blocks.
        int pos2 = 0;
        for (auto& replctrl : RA_replay_ctrls) {
            for (auto& rctrl : RA_radio_ctrls) {
                RA_graph->connect(replctrl->get_block_id(),
                    RA_replay_chan_vector[pos2],
                    rctrl->get_block_id(),
                    0);
                pos2++;
            }
        }
    }
    for (size_t i_s2r = 0; i_s2r < RA_replay_ctrls.size(); i_s2r = i_s2r + 2) {
        RA_graph->connect(
            RA_tx_stream_vector[i_s2r], 0, RA_replay_ctrls[i_s2r]->get_block_id(), 0);
        std::cout << "Streamer: " << RA_tx_stream_vector[i_s2r] << " connected to "
                  << RA_replay_ctrls[i_s2r]->get_block_id() << std::endl;
    }
}
void RefArch::connectGraphMultithreadHostTX()
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
    }
    // Vector of streamer channels.
    for (size_t i_chan = 0; i_chan < RA_rx_stream_vector.size() * 2; i_chan++) {
        if (i_chan % 2 == 0) {
            RA_rx_stream_chan_vector.push_back(0);
        } else {
            RA_rx_stream_chan_vector.push_back(1);
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
    }
    int pos2 = 0;

    for (auto& rctrl : RA_radio_ctrls) {
        RA_graph->connect(
            RA_duc_ctrls[pos2]->get_block_id(), RA_duc_chan, rctrl->get_block_id(), 0);
        std::cout << "Connected " << RA_duc_ctrls[pos2]->get_block_id() << " port "
                  << RA_duc_chan << " to radio " << rctrl->get_block_id() << " port " << 0
                  << std::endl;

        RA_graph->connect(
            RA_tx_stream_vector[pos2], 0, RA_duc_ctrls[pos2]->get_block_id(), 0);
        std::cout << "Streamer: " << RA_tx_stream_vector[pos2] << " connected to "
                  << RA_replay_ctrls[pos2]->get_block_id() << std::endl;
        pos2++;
    }
}
void RefArch::buildStreamsMultithread()
{
    // TODO: Think about renaming
    // Build Streams for multithreaded implementation streaming from Replay Block.
    // Each Channel gets its own RX streamer.
    // Constants related to the Replay block
    const size_t replay_word_size = 8; // Size of words used by replay block
    const size_t sample_size      = 4; // Complex signed 16-bit is 32 bits per sample
    const size_t samples_per_word = 2; // Number of sc16 samples per word
    uhd::device_addr_t streamer_args(RA_streamargs);
    // create a receive streamer
    uhd::stream_args_t stream_args(RA_format, RA_otw);
    stream_args.args = streamer_args;
    std::cout << "Using streamer args: " << stream_args.args.to_string() << std::endl;
    // One stream per channel
    for (size_t rx_count = 0; rx_count < RA_radio_ctrls.size() / 2; rx_count++) {
        RA_rx_stream = RA_graph->create_rx_streamer(2, stream_args);
        RA_rx_stream_vector.push_back(RA_rx_stream);
        RA_rx_stream_vector.push_back(RA_rx_stream);
    }
    /************************************************************************
     * Set up streamer to Replay blocks
     ***********************************************************************/
    for (size_t i_s2r = 0; i_s2r < RA_replay_ctrls.size(); i_s2r = i_s2r + 2) {
        streamer_args["block_id"]   = RA_replay_ctrls[i_s2r]->get_block_id().to_string();
        streamer_args["block_port"] = std::to_string(0);
        stream_args.args            = streamer_args;
        stream_args.channels        = {0};
        RA_tx_stream =
            RA_graph->create_tx_streamer(stream_args.channels.size(), stream_args);
        size_t tx_spp = RA_tx_stream->get_max_num_samps();
        // Make sure that stream SPP is a multiple of the Replay Block Word Size
        if (tx_spp % samples_per_word != 0) {
            // Round SPP down to a multiple of the word size
            tx_spp               = (tx_spp / samples_per_word) * samples_per_word;
            streamer_args["spp"] = std::to_string(tx_spp);
            stream_args.args     = streamer_args;
            RA_tx_stream =
                RA_graph->create_tx_streamer(stream_args.channels.size(), stream_args);
        }
        // Vector of tx streamers, duplicate for vector length padding.
        RA_tx_stream_vector.push_back(RA_tx_stream);
        RA_tx_stream_vector.push_back(RA_tx_stream);
    }
}
void RefArch::buildStreamsMultithreadHostTX()
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
    for (int rx_count = 0; rx_count < RA_radio_ctrls.size() / 2; rx_count++) {
        RA_rx_stream = RA_graph->create_rx_streamer(2, stream_args);
        RA_rx_stream_vector.push_back(RA_rx_stream);
        RA_rx_stream_vector.push_back(RA_rx_stream);
    }
    /************************************************************************
     * Set up streamer to Replay blocks
     ***********************************************************************/
    for (int i_s2r = 0; i_s2r < RA_radio_ctrls.size(); i_s2r = i_s2r + 1) {
        streamer_args["block_id"]   = RA_duc_ctrls[i_s2r]->get_block_id().to_string();
        streamer_args["block_port"] = std::to_string(0);
        stream_args.args            = streamer_args;
        stream_args.channels        = {0};

        RA_tx_stream =
            RA_graph->create_tx_streamer(stream_args.channels.size(), stream_args);
        size_t tx_spp = RA_tx_stream->get_max_num_samps();
        // Make sure that stream SPP is a multiple of the Replay Block Word Size
        if (tx_spp % samples_per_word != 0) {
            // Round SPP down to a multiple of the word size
            tx_spp               = (tx_spp / samples_per_word) * samples_per_word;
            streamer_args["spp"] = std::to_string(tx_spp);
            stream_args.args     = streamer_args;
            RA_tx_stream =
                RA_graph->create_tx_streamer(stream_args.channels.size(), stream_args);
        }
        // Vector of tx streamers
        RA_tx_stream_vector.push_back(RA_tx_stream);
    }
}

// blocksettings
int RefArch::setRadioRates()
{
    /************************************************************************
     * Set up radio, DDCs, and DUCs
     ***********************************************************************/
    // set the RX sample rate
    if (RA_rx_rate <= 0.0) {
        std::cerr << "Please specify a valid RX sample rate" << std::endl;
        return EXIT_FAILURE;
    }
    // set the TX sample rate
    if (RA_tx_rate <= 0.0) {
        std::cerr << "Please specify a valid TX sample rate" << std::endl;
        return EXIT_FAILURE;
    }
    // Set DDC & DUC Sample Rates
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (RA_rx_rate / 1e6)
              << std::endl;
    // set rates for each DDC Block
    // TODO: Look at rates here. rate == ....
    // TODO: Fix Display
    if (RA_ddc_ctrls.size() > 0) {
        int count_rx = 0;
        for (uhd::rfnoc::ddc_block_control::sptr ddcctrl : RA_ddc_ctrls) {
            std::cout << "DDC block found" << std::endl;
            double radio_rate = RA_radio_ctrls[count_rx]->get_rate();
            int decim         = (int)(radio_rate / RA_rx_rate);
            std::cout << boost::format("Setting decimation value to %d") % decim
                      << std::endl;
            ddcctrl->set_property<int>("decim", decim, RA_ddc_chan);
            decim = ddcctrl->get_property<int>("decim", RA_ddc_chan);
            std::cout << boost::format("Actual decimation value is %d") % decim
                      << std::endl;
            RA_rx_rate = radio_rate / decim;
            count_rx++;
        }
    } else {
        for (auto& rctrl : RA_radio_ctrls) {
            RA_rx_rate = rctrl->set_rate(RA_rx_rate);
            std::cout << boost::format("Actual RX Rate: %f Msps...") % (RA_rx_rate / 1e6)
                      << std::endl
                      << std::endl;
        }
    }
    std::cout << "Actual RX Rate: " << (RA_rx_rate / 1e6) << " Msps..." << std::endl
              << std::endl;
    std::cout << std::resetiosflags(std::ios::fixed);
    std::cout << "Setting TX Rate: " << (RA_tx_rate / 1e6) << " Msps..." << std::endl;
    if (RA_duc_ctrls.size() > 0) {
        int radio = 0;
        for (uhd::rfnoc::duc_block_control::sptr dctrl : RA_duc_ctrls) {
            std::cout << "DUC block found." << dctrl->get_block_id() << std::endl;
            dctrl->set_input_rate(RA_tx_rate, RA_duc_chan);
            dctrl->set_output_rate(RA_radio_ctrls[radio]->get_rate(), RA_duc_chan);
            std::cout << dctrl->get_block_id() << " Interpolation value is "
                      << dctrl->get_property<int>("interp", RA_duc_chan) << std::endl;
            RA_tx_rate = dctrl->get_input_rate(RA_duc_chan);
            radio++;
        }
    } else {
        for (uhd::rfnoc::radio_control::sptr rctrl : RA_radio_ctrls) {
            RA_tx_rate = rctrl->set_rate(RA_tx_rate);
        }
    }
    std::cout << "Actual TX Rate: " << (RA_tx_rate / 1e6) << " Msps..." << std::endl
              << std::endl
              << std::resetiosflags(std::ios::fixed);
    return EXIT_SUCCESS;
}
void RefArch::tuneRX()
{
    for (auto& rctrl : RA_radio_ctrls) {
        // Set USRP RX Frequency for All Devices
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting RX Freq: " << std::fixed
                  << (RA_rx_freq / 1e6) << " MHz..." << std::endl;

        rctrl->set_rx_frequency(RA_rx_freq, 0);
        std::cout << rctrl->get_block_id()
                  << " Actual RX Freq: " << (rctrl->get_rx_frequency(0) / 1e6)
                  << " MHz..." << std::endl
                  << std::endl;
    }
}
void RefArch::tuneTX()
{
    for (auto& rctrl : RA_radio_ctrls) {
        // Set USRP TX Frequency for All devices
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting TX Freq: " << std::fixed
                  << (RA_tx_freq / 1e6) << " MHz..." << std::endl;
        rctrl->set_tx_frequency(RA_tx_freq, 0);
        std::cout << rctrl->get_block_id()
                  << " Actual TX Freq: " << (rctrl->get_tx_frequency(0) / 1e6)
                  << " MHz..." << std::endl
                  << std::endl;
    }
}
void RefArch::setRXGain()
{
    // Set RX Gain of all Devices (Appears that max in UHD Is 65)
    for (auto& rctrl : RA_radio_ctrls) {
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting RX Gain: " << RA_rx_gain
                  << " dB..." << std::endl;
        rctrl->set_rx_gain(RA_rx_gain, 0);
        std::cout << rctrl->get_block_id() << " Actual RX Gain: " << rctrl->get_rx_gain(0)
                  << " dB..." << std::endl
                  << std::endl;
        std::cout << std::resetiosflags(std::ios::fixed);
    }
}
void RefArch::setTXGain()
{
    // Set TX Gain of all devices (Appears that max in UHD is 65)
    for (auto& rctrl : RA_radio_ctrls) {
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting TX Gain: " << RA_tx_gain
                  << " dB..." << std::endl;
        rctrl->set_tx_gain(RA_tx_gain, 0);
        std::cout << rctrl->get_block_id() << " Actual TX Gain: " << rctrl->get_tx_gain(0)
                  << " dB..." << std::endl
                  << std::endl;
        std::cout << std::resetiosflags(std::ios::fixed);
    }
}
void RefArch::setRXBw()
{
    // Set RX BandWidth for all devices
    if (RA_rx_bw > 0) {
        for (auto& rctrl : RA_radio_ctrls) {
            std::cout << std::fixed;
            std::cout << rctrl->get_block_id()
                      << " Setting RX Bandwidth: " << (RA_rx_bw / 1e6) << " MHz..."
                      << std::endl;
            rctrl->set_rx_bandwidth(RA_rx_bw, 0);
            std::cout << rctrl->get_block_id()
                      << " Actual RX Bandwidth: " << (rctrl->get_rx_bandwidth(0) / 1e6)
                      << " MHz..." << std::endl
                      << std::endl;
            std::cout << std::resetiosflags(std::ios::fixed);
        }
    }
}
void RefArch::setTXBw()
{
    // Set TX BandWidth for all devices
    if (RA_tx_bw > 0) {
        for (auto& rctrl : RA_radio_ctrls) {
            std::cout << std::fixed;
            std::cout << rctrl->get_block_id()
                      << " Setting TX Bandwidth: " << (RA_tx_bw / 1e6) << " MHz..."
                      << std::endl;
            rctrl->set_tx_bandwidth(RA_tx_bw, 0);
            std::cout << rctrl->get_block_id()
                      << " Actual TX Bandwidth: " << (rctrl->get_tx_bandwidth(0) / 1e6)
                      << " MHz..." << std::endl
                      << std::endl;
            std::cout << std::resetiosflags(std::ios::fixed);
        }
    }
}
void RefArch::setRXAnt()
{
    // Set RX Antenna for all devices
    for (auto& rctrl : RA_radio_ctrls) {
        rctrl->set_rx_antenna(RA_rx_ant, 0);
    }
}
void RefArch::setTXAnt()
{
    // Set TX Antenna for all devices
    for (auto& rctrl : RA_radio_ctrls) {
        rctrl->set_tx_antenna(RA_tx_ant, 0);
    }
}
// recvdata
void RefArch::recv(int rx_channel_nums, int threadnum, uhd::rx_streamer::sptr rx_streamer)
{
    // Receive to memory only, multi-threaded implementation.
    uhd::set_thread_priority_safe(0.9F);
    int num_total_samps = 0;
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
    uhd::stream_cmd_t stream_cmd((RA_nsamps == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = RA_nsamps;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = RA_start_time;
    md.has_time_spec      = true;
    md.time_spec          = RA_start_time;
    const auto stop_time  = RA_start_time + uhd::time_spec_t(RA_time_requested);
    rx_streamer->issue_stream_cmd(stream_cmd);
    int loop_num = 0;
    while (not RA_stop_signal_called and (RA_nsamps > num_total_samps or RA_nsamps == 0)
           and (RA_time_requested == 0.0
                or RA_graph->get_mb_controller(0)->get_timekeeper(0)->get_time_now()
                       <= stop_time)) {
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
                           "Got an overflow indication. Please consider the following:\n"
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
            break;
        }
        num_total_samps += num_rx_samps * rx_streamer->get_num_channels();
    }
    // Shut down receiver
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_streamer->issue_stream_cmd(stream_cmd);
    std::cout << "Thread: " << threadnum << " Received: " << num_total_samps
              << " samples..." << std::endl;
}
void RefArch::transmitFromFile(
    uhd::tx_streamer::sptr tx_streamer, uhd::tx_metadata_t metadata, int num_channels)
{
    uhd::set_thread_priority_safe(0.9F);
    std::vector<std::complex<float>> buff(RA_spb);
    std::vector<std::complex<float>*> buffs(num_channels, &buff.front());
    std::ifstream infile(RA_file.c_str(), std::ifstream::binary);
    // send data until  the signal handler gets called
    while (not RA_stop_signal_called) {
        infile.read((char*)&buff.front(), buff.size() * sizeof(int16_t));
        size_t num_tx_samps = size_t(infile.gcount() / sizeof(int16_t));

        metadata.end_of_burst = infile.eof();
        // send the entire contents of the buffer
        tx_streamer->send(buffs, buff.size(), metadata);

        metadata.start_of_burst = false;
        metadata.has_time_spec  = false;
    }

    // send a mini EOB packet
    metadata.end_of_burst = true;
    tx_streamer->send("", 0, metadata);
}
void RefArch::transmitFromReplay()
{
    // TODO: Separate out replay TX
    std::cout << "Replaying data (Press Ctrl+C to stop)..." << std::endl;
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    if (RA_nsamps <= 0) {
        // replay the entire buffer over and over
        std::cout << "Issuing replay command for " << RA_samples_to_replay
                  << " samps in continuous mode..." << std::endl;
    } else {
        // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
        stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;
    }
    if (RA_TX_All_Chan == true) {
        for (int i = 0; i < RA_replay_ctrls.size(); i = i + 2) {
            // Replay Block Channel (output) 0
            std::cout << RA_replay_ctrls[i]->get_block_id()
                      << " Port: " << RA_replay_chan_vector[i] << std::endl
                      << RA_replay_ctrls[i]->get_block_id()
                      << " Issuing replay command for " << RA_nsamps << " samps..."
                      << std::endl;
            RA_replay_ctrls[i]->config_play(
                RA_replay_buff_addr, RA_replay_buff_size, RA_replay_chan_vector[i]);
            stream_cmd.num_samps  = RA_nsamps;
            stream_cmd.stream_now = false;
            stream_cmd.time_spec  = RA_start_time;
            RA_replay_ctrls[i]->issue_stream_cmd(stream_cmd, RA_replay_chan_vector[i]);
            // Replay Block Channel (output) 1
            std::cout << RA_replay_ctrls[i]->get_block_id()
                      << " Port: " << RA_replay_chan_vector[i + 1] << std::endl
                      << RA_replay_ctrls[i]->get_block_id()
                      << " Issuing replay command for " << RA_nsamps << " samps..."
                      << std::endl;
            RA_replay_ctrls[i]->config_play(
                RA_replay_buff_addr, RA_replay_buff_size, RA_replay_chan_vector[i + 1]);
            stream_cmd.num_samps  = RA_nsamps;
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
                  << " Issuing replay command for " << RA_nsamps << " samps..."
                  << std::endl;
        RA_replay_ctrls[RA_singleTX]->config_play(
            RA_replay_buff_addr, RA_replay_buff_size, RA_replay_chan_vector[RA_singleTX]);
        stream_cmd.num_samps  = RA_nsamps;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec  = RA_start_time;
        RA_replay_ctrls[RA_singleTX]->issue_stream_cmd(
            stream_cmd, RA_replay_chan_vector[RA_singleTX]);
    }
}
void RefArch::spawnTransmitThreads()
{
    if (RA_spb == 0)
        RA_spb = RA_tx_stream_vector[0]->get_max_num_samps() * 10;
    // setup the metadata flags
    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = true;
    md.time_spec      = RA_start_time;
    if (RA_TX_All_Chan == true) {
        for (size_t i = 0; i < RA_tx_stream_vector.size(); i++) {
            // start transmit worker thread, not for use with replay block.
            std::cout << "Spawning TX thread: " << i << std::endl;
            std::thread tx(
                [this](uhd::tx_streamer::sptr tx_streamer,
                    uhd::tx_metadata_t metadata,
                    int num_channels) {
                    transmitFromFile(tx_streamer, metadata, num_channels);
                },
                RA_tx_stream_vector[i],
                md,
                1);
            RA_tx_vector_thread.push_back(std::move(tx));
        }
    } else {
        // start transmit worker thread, not for use with replay block.
        std::cout << "Spawning Single TX thread, Channel: " << RA_singleTX << std::endl;
        std::thread tx(
            [this](uhd::tx_streamer::sptr tx_streamer,
                uhd::tx_metadata_t metadata,
                int num_channels) {
                transmitFromFile(tx_streamer, metadata, num_channels);
            },
            RA_tx_stream_vector[RA_singleTX],
            md,
            1);
        RA_tx_vector_thread.push_back(std::move(tx));
    }
}
void RefArch::spawnReceiveThreads()
{
    int threadnum = 0;


    // Receive RA_rx_stream_vector.size()
    if (RA_format == "sc16") {
        for (size_t i = 0; i < RA_rx_stream_vector.size(); i = i + 2) {
            std::cout << "Spawning RX Thread.." << threadnum << std::endl;
            std::thread t(
                [this](int threadnum, uhd::rx_streamer::sptr rx_streamer) {
                    recv(2, threadnum, rx_streamer);
                },
                threadnum,
                RA_rx_stream_vector[i]);

            RA_rx_vector_thread.push_back(std::move(t));
            threadnum++;
        }
    } else {
        throw std::runtime_error("Unknown type " + RA_format);
    }
    return;
}
void RefArch::joinAllThreads()
{
    // Joins RX and TX threads if they exist.
    std::cout << "Waiting to join threads.." << std::endl;
    // Join RX Threads
    for (auto& rx : RA_rx_vector_thread) {
        rx.join();
    }
    RA_rx_vector_thread.clear();
    // Stop Transmitting once RX is complete
    bool temp_stop_signal = RA_stop_signal_called;
    RA_stop_signal_called = true;
    // Join TX Threads
    for (auto& tx : RA_tx_vector_thread) {
        tx.join();
    }
    RA_tx_vector_thread.clear();
    std::cout << "Threads Joined" << std::endl;
    RA_stop_signal_called = temp_stop_signal; // return stop_signal_called
}
void RefArch::spawnTimer(){
    int threadnum = 1; 
    std::cout << "Spawning Timer Thread" << std::endl;
    std::thread t([this](int threadnum){asyncTimer(threadnum);},threadnum);
    RA_timerthread = std::move(t);
    return;
}
//Async Timer
void RefArch::asyncTimer(int threadnum){
  std::cout << "Entering Thread" << std::flush;
  int time =0;
  while (RA_stop_signal_called == false and time <= RA_time_requested){
    std::this_thread::sleep_for(std::chrono::seconds(1));
    time++;
  };
  RA_stop_signal_called = true;
}
