#include "ArchUSRP.hpp"
#include <boost/filesystem.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/circular_buffer.hpp>
#include <thread>
#include <fstream>


ArchUSRP::ArchUSRP(int argc, char* argv[]){
    this->addProgramOptions();
    this->storeProgramOptions(argc, argv);
    this->addAddresstoArgs();
}

//structures
void ArchUSRP::addProgramOptions(){
    namespace po = boost::program_options;
        // clang-format off
        //TODO: Verify we are still using the comments for each value in
        //or if we can delete and push explaination to top of each structure

        desc.add_options()
        ("cfgFile", po::value<std::string>(&cfgFile), "relative path to configuration file")
        ("args", po::value<std::string>(&args)->default_value(""), "uhd transmit device address args")
        ("tx-rate", po::value<double>(&tx_rate)->default_value(200e6), "rate of transmit outgoing samples")
        ("rx-rate", po::value<double>(&rx_rate)->default_value(25e6), "rate of receive incoming samples")
        ("tx-gain", po::value<double>(&tx_gain)->default_value(0), "gain for the transmit RF chain")
        ("rx-gain", po::value<double>(&rx_gain)->default_value(0), "gain for the receive RF chain")
        ("ref", po::value<std::string>(&ref)->default_value("external"), "clock reference (internal, external, mimo)")
        ("tx-freq", po::value<double>(&tx_freq)->default_value(2000e6), "transmit RF center frequency in Hz")
        ("rx-freq", po::value<double>(&rx_freq)->default_value(2000e6), "receive RF center frequency in Hz")
        ("address", po::value<std::vector<std::string>>(&address), "uhd transmit device address args")
        ("tx-ant", po::value<std::string>(&tx_ant)->default_value("TX/RX"), "transmit antenna selection")
        ("rx-ant", po::value<std::string>(&rx_ant)->default_value("RX2"), "receive antenna selection")
        ("streamargs", po::value<std::string>(&streamargs)->default_value(""), "stream args")
        ("tx-bw", po::value<double>(&tx_bw)->default_value(0), "analog transmit filter bandwidth in Hz")
        ("rx-bw", po::value<double>(&rx_bw)->default_value(0), "analog receive filter bandwidth in Hz")
        ("lo", po::value<std::vector<std::string>>(&lo), "device LO settings")
        ("rx-file", po::value<std::string>(&rx_file)->default_value("test.dat"), "name of the file to write binary samples to")
        ("rx-file-location", po::value<std::vector<std::string>>(&rx_file_location))
        ("rx-file-streamers",po::value<std::vector<std::string>>(&rx_file_streamers))
        ("otw", po::value<std::string>(&otw)->default_value("sc16"), "specify the over-the-wire sample mode")
        ("type", po::value<std::string>(&type)->default_value("short"), "sample type in file: double, float, or short")
        ("spb", po::value<size_t>(&spb)->default_value(0), "samples per buffer, 0 for default")
        ("rx_timeout", po::value<double>(&rx_timeout)->default_value(double(3)), "number of seconds before rx streamer times out")
        ("format", po::value<std::string>(&format)->default_value("sc16"), "File sample format: sc16, fc32, or fc64")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to transmit")
        ("nsamps", po::value<size_t>(&nsamps)->default_value(16000), "number of samples to play (0 for infinite)")
        ("replay_time",po::value<double>(&rtime)->default_value(2.0), "Replay Block Time Delay (seconds)")
        ("nruns", po::value<size_t>(&nruns)->default_value(1), "number of repeats")
        ("repeat_delay", po::value<double>(&rep_delay)->default_value(0), "delay between repeats (seconds)")
        ("time_adjust",po::value<double>(&time_adjust)->default_value(2.0), "If tramsmitting in iterative mode, seperation between per-channel transmission (seconds).")
        ("singleTX",po::value<int>(&singleTX)->default_value(0), "TX Channel)")
        ("time_requested", po::value<double>(&time_requested)->default_value(0.0), "Single Loopback Continous Time Limit (s).")
        ;
        // clang-format on
}
void ArchUSRP::addAddresstoArgs(){
    argsWithAddress = "" + args;
    for (const auto& addr : address) {
        argsWithAddress += ", " + addr;
    }
    args = argsWithAddress;
}
void ArchUSRP::storeProgramOptions(int argc, char* argv[]){
    namespace po = boost::program_options;
    // store program options from cmd line
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    // store program options from config file
    if (vm.count("cfgFile")) {
        std::cout << "Load cfg_file: " << cfgFile << std::endl;
        // standard streams don't accept a standard string, so pass the string using
        // c_str()
        po::store(po::parse_config_file(cfgFile.c_str(), desc), vm);
        po::notify(vm);
    }
}
//sync
void ArchUSRP::setSources(){
    // Set clock reference
    std::cout << "Locking motherboard reference/time sources..." << std::endl;
    // Lock mboard clocks
    for (size_t i = 0; i < graph->get_num_mboards(); ++i) {
        graph->get_mb_controller(i)->set_clock_source(ref);
        graph->get_mb_controller(i)->set_time_source(ref);
    }
}
int ArchUSRP::syncAllDevices()
{
    // Synchronize Devices
    bool sync_result;
    const uhd::time_spec_t timeSpec = 0.0;
    sync_result = graph->synchronize_devices(timeSpec, true);
    // wait for one sec to ensure rising edge is found
    //TODO: Double check this
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Synchronized" << std::endl;
    if (sync_result != true) {
        std::cout << "Unable to Synchronize Devices " << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
void ArchUSRP::killLOs(){
    std::cout << "Shutting Down LOs" << std::endl;
    int device = 0;
    while (device < lo.size()) {
        if (lo[device] == "source" or lo[device] == "distributor") {
            graph->get_tree()->access<bool>(
                str(boost::format("%s%d%s") % "blocks/" % device % 
                "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export"))
                .set(false);
            graph->get_tree()->access<bool>(
                str(boost::format("%s%d%s") % "blocks/" % device % 
                "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export"))
                .set(false);
            graph->get_tree()->access<bool>(
                str(boost::format("%s%d%s") % "blocks/" % device % 
                "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export"))
                .set(false);
            graph->get_tree()->access<bool>(
                str(boost::format("%s%d%s") % "blocks/" % device % 
                "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export"))
                .set(false);

            graph->get_tree()->access<bool>(
                str(boost::format("%s%d%s") % "blocks/" % device % 
                "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export"))
                .set(false);
            graph->get_tree()->access<bool>(
                str(boost::format("%s%d%s") % "blocks/" % device % 
                "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export"))
                .set(false);
            graph->get_tree()->access<bool>(
                str(boost::format("%s%d%s") % "blocks/" % device % 
                "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export"))
                .set(false);
            graph->get_tree()->access<bool>(
                str(boost::format("%s%d%s") % "blocks/" % device % 
                "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export"))
                .set(false);
        }
        device++;
    }
    std::cout << "Shutting Down LOs: Done!" << std::endl;
}
void ArchUSRP::setLOsfromConfig()
{
    // Set LOs per config from config file
    int device = 0;
    while (device < lo.size()) {
        if (lo[device] == "source") {
            ArchUSRP::setSource(device);
        } else if (lo[device] == "terminal") {
            ArchUSRP::setTerminal(device);
        } else if (lo[device] == "distributor") {
            ArchUSRP::setDistributor(device);
        }
    device++;
    }
}
void ArchUSRP::setSource(int device)
{
    std::cout << "Setting Device# " << device << " Radio# " << device * 2 << ", Radio# "
              << device * 2 + 1 << " to: "
              << "source" << std::endl;
    // Set Device to System LO Source
    // No difference between RX and TX LOs, just used RX.
    // TODO: Hardcoded number of channels per device.
    radio_ctrls[device * 2]->set_tx_lo_export_enabled(false, "lo1", 0);
    radio_ctrls[device * 2 + 1]->set_tx_lo_export_enabled(false, "lo1", 0);

    radio_ctrls[device * 2]->set_rx_lo_export_enabled(true, "lo1", 0);
    radio_ctrls[device * 2 + 1]->set_rx_lo_export_enabled(true, "lo1", 0);

    // TODO: this did not clean up nice. Reformat.
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export"))
        .set(true);

    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export"))
        .set(true);

    radio_ctrls[device * 2]->set_tx_lo_source("external", "lo1", 0);
    radio_ctrls[device * 2 + 1]->set_tx_lo_source("external", "lo1", 0);

    radio_ctrls[device * 2]->set_rx_lo_source("external", "lo1", 0);
    radio_ctrls[device * 2 + 1]->set_rx_lo_source("external", "lo1", 0);
}
void ArchUSRP::setTerminal(int device)
{
    // TODO: Hardcoded number of channels per device.
    std::cout << "Setting Device# " << device << " Radio# " << device * 2 << ", Radio# "
              << device * 2 + 1 << " to: "
              << "terminal" << std::endl;
    radio_ctrls[device * 2]->set_tx_lo_source("external", "lo1", 0);
    radio_ctrls[device * 2 + 1]->set_tx_lo_source("external", "lo1", 0);

    radio_ctrls[device * 2]->set_rx_lo_source("external", "lo1", 0);
    radio_ctrls[device * 2 + 1]->set_rx_lo_source("external", "lo1", 0);
}
void ArchUSRP::setDistributor(int device)
{
    // TODO: Hardcoded number of channels per device.
    std::cout << "Setting Device# " << device << " Radio# " << device * 2 << ", Radio# "
              << device * 2 + 1 << " to: "
              << "distributor" << std::endl;

    radio_ctrls[device * 2]->set_tx_lo_source("external", "lo1", 0);
    radio_ctrls[device * 2 + 1]->set_tx_lo_source("external", "lo1", 0);

    radio_ctrls[device * 2]->set_rx_lo_source("external", "lo1", 0);
    radio_ctrls[device * 2 + 1]->set_rx_lo_source("external", "lo1", 0);

    radio_ctrls[device * 2]->set_tx_lo_export_enabled(false, "lo1", 0);
    radio_ctrls[device * 2 + 1]->set_tx_lo_export_enabled(false, "lo1", 0);

    radio_ctrls[device * 2]->set_rx_lo_export_enabled(false, "lo1", 0);
    radio_ctrls[device * 2 + 1]->set_rx_lo_export_enabled(false, "lo1", 0);

    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export"))
        .set(true);

    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export"))
        .set(true);
    graph->get_tree()->access<bool>(
        str(boost::format("%s%d%s") % "blocks/" % device % 
        "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export"))
        .set(true);

}
void ArchUSRP::checkRXSensorLock()
{
    // Check Locked RX Sensors
    std::vector<std::string> rx_sensor_names;
    for (auto& rctrl : radio_ctrls) {
        rx_sensor_names = rctrl->get_rx_sensor_names(0);
        for (auto& name : rx_sensor_names) {
            uhd::sensor_value_t rx_sensor_value = rctrl->get_rx_sensor(name, 0);
            std::cout << "Checking RX LO Lock: " << rx_sensor_value.to_pp_string()
                      << std::endl;
            while (rx_sensor_value.to_pp_string() != "all_los: locked") {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "RX LO LOCKED" << std::endl;
        }
    }
}
void ArchUSRP::checkTXSensorLock()
{
    // Check Locked TX Sensors
    std::vector<std::string> tx_sensor_names;
    for (auto& rctrl : radio_ctrls) {
        tx_sensor_names = rctrl->get_tx_sensor_names(0);
        for (auto& name : tx_sensor_names) {
            uhd::sensor_value_t tx_sensor_value = rctrl->get_tx_sensor(name, 0);
            std::cout << "Checking TX LO Lock: " << tx_sensor_value.to_pp_string()
                      << std::endl;
            while (tx_sensor_value.to_pp_string() != "all_los: locked") {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "TX LO LOCKED" << std::endl;
        }
    }
}
//replaycontrol
int ArchUSRP::importData()
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
    replay_buff_addr = 0;
    replay_buff_size = samples_to_replay * sample_size;

    // Create buffer
    std::vector<char> tx_buffer(samples_to_replay * sample_size);
    char* tx_buf_ptr = &tx_buffer[0];

    // Read file into buffer, rounded down to number of words
    infile.read(tx_buf_ptr, samples_to_replay * sample_size);
    infile.close();
    for (int i = 0; i < replay_ctrls.size(); i = i + 2) {
        /************************************************************************
         * Configure replay block
         ***********************************************************************/
        // Configure a buffer in the on-board memory at address 0 that's equal in
        // size to the file we want to play back (rounded down to a multiple of
        // 64-bit words). Note that it is allowed to playback a different size or
        // location from what was recorded.
        std::cout << replay_ctrls[i]->get_block_id() << std::endl;
        replay_ctrls[i]->record(
            replay_buff_addr, replay_buff_size, 0);
        // Display replay configuration
        std::cout << "Replay file size:     " << replay_buff_size
                  << " bytes (" << words_to_replay << " qwords, " << samples_to_replay
                  << " samples)" << std::endl
                  << "Record base address:  0x" << std::hex
                  << replay_ctrls[i]->get_record_offset(0) << std::dec
                  << std::endl
                  << "Record buffer size:   "
                  << replay_ctrls[i]->get_record_size(0) << " bytes"
                  << std::endl
                  << "Record fullness:      "
                  << replay_ctrls[i]->get_record_fullness(0) << " bytes"
                  << std::endl
                  << std::endl;
        // Restart record buffer repeatedly until no new data appears on the Replay
        // block's input. This will flush any data that was buffered on the input.
        uint32_t fullness;
        std::cout << "Emptying record buffer..." << std::endl;
        do {
            std::chrono::system_clock::time_point time_spec;
            std::chrono::system_clock::duration time_diff;
            replay_ctrls[i]->record_restart(0);
            // Make sure the record buffer doesn't start to fill again
            time_spec = std::chrono::system_clock::now();
            do {
                fullness = replay_ctrls[i]->get_record_fullness(0);
                if (fullness != 0)
                    std::cout << "BREAK" << std::endl;
                break;
                time_diff = std::chrono::system_clock::now() - time_spec;
                time_diff =
                    std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);
            } while (time_diff.count() < 250);
        } while (fullness);
        std::cout << "Record fullness:      "
                  << replay_ctrls[i]->get_record_fullness(0) << " bytes"
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
            tx_stream_vector[i]->send(tx_buf_ptr, samples_to_replay, tx_md);
        if (num_tx_samps != samples_to_replay) {
            std::cout << "ERROR: Unable to send " << samples_to_replay << " samples"
                      << std::endl;
            return EXIT_FAILURE;
        }

        /************************************************************************
         * Wait for data to be stored in on-board memory
         ***********************************************************************/
        std::cout << "Waiting for recording to complete..." << std::endl;
        while (replay_ctrls[i]->get_record_fullness(0)
               < replay_buff_size);
        std::cout << "Record fullness:      "
                  << replay_ctrls[i]->get_record_fullness(0) << " bytes"
                  << std::endl
                  << std::endl;
    }
    return EXIT_SUCCESS;
}
void ArchUSRP::stopReplay()
{
    /************************************************************************
     * Issue stop command
     ***********************************************************************/
    std::cout << "Stopping replay..." << std::endl;
    for (int i_kill = 0; i_kill < replay_ctrls.size(); i_kill++) {
        replay_ctrls[i_kill]->stop(
            replay_chan_vector[i_kill]);
    }
}
void ArchUSRP::sig_int_handler(int)
{
    stop_signal_called = true;
}
//receivefunctions
std::map<int,std::string> ArchUSRP::getStreamerFileLocation(
    std::vector<std::string> rx_file_streamers,
    std::vector<std::string> rx_file_location){
    std::map<int,std::string> usrpFileMap;
    for(int i =0; i< rx_file_streamers.size(); i++){
        size_t pos = 0;
        int n;
        std::string usrpStringArray = rx_file_streamers[i];
        while (1) {
            try {
                int n = std::stoi(usrpStringArray,&pos);
                usrpFileMap[n]=rx_file_location[i];
                usrpStringArray = usrpStringArray.substr(pos);
            }
            catch (...) {break;}
        }
    }
    return usrpFileMap;
}
std::string ArchUSRP::generateRxFilename(const std::string& base_fn,
    const size_t& rx_chan_num,
    const int& tx_chan_num,
    const int& run_num,
    const double& tx_freq,
    const std::string folder_name,
    const std::vector<std::string> rx_streamer_string,
    const std::vector<std::string> rx_file_location)
{
    // Generates filenames for multithreaded implementation.
    if (rx_file_location.size() != rx_streamer_string.size()){
        throw std::runtime_error(
            "The number of Streamers file locations must match the number of Folder Names");
    }
    std::map<int,std::string> streamer_files = getStreamerFileLocation(rx_streamer_string, rx_file_location);
    std::string cw_folder =
        "CW_" + std::to_string(tx_freq * 1e-9) + "_GHz_" + folder_name;
    boost::filesystem::create_directory(
        str(boost::format("%s%s") % streamer_files[rx_chan_num] % cw_folder));
    boost::filesystem::path base_fn_fp(streamer_files[rx_chan_num] + cw_folder + "/" + base_fn);
    base_fn_fp.replace_extension(boost::filesystem::path(
        str(boost::format("%s%02d%s%02d%s%02d%s%02d%s") % "tx_" % tx_chan_num
            % "_rx_" % rx_chan_num % "_run_" % run_num % "_cw_" % tx_freq 
            % base_fn_fp.extension().string())));
    return base_fn_fp.string();
}
//graphassembly
void ArchUSRP::buildGraph()
{
    /************************************************************************
     * Create device and block controls
     ***********************************************************************/
    // If multiple USRPs are used, they are linked into a single RFNoc graph here.
    std::cout << std::endl;
    std::cout << "Creating the RFNoC graph with args: " << args << "..." << std::endl;
    graph = uhd::rfnoc::rfnoc_graph::make(args);
}
void ArchUSRP::buildRadios()
{
    /************************************************************************
     * Seek radio blocks on each USRP and assemble a vector of radio
     * controllers.
     ***********************************************************************/
    std::cout << "Building Radios..." << std::endl;
    // Make radio controllers for multiple channels/devices
    radio_block_list = graph->find_blocks("Radio");
    // Iterate over each radio block found on each device
    for (auto& elem : radio_block_list) {
        // Create a vector of radio control objects for controlling the radio blocks
        radio_ctrls.push_back(
            graph->get_block<uhd::rfnoc::radio_control>(elem));
        std::cout << "Using radio " << elem << std::endl;
    }
    // Sort the vectors
    sort(radio_ctrls.begin(), radio_ctrls.end());
    sort(radio_block_list.begin(), radio_block_list.end());
}
void ArchUSRP::buildDDCDUC()
{
    /*************************************************************************
     * Seek DDCs & DUCs on each USRP and assemble a vector of DDC & DUC controllers.
     ************************************************************************/
    std::cout << "Building DDC/DUCs.." << std::endl;
    std::vector<std::string> ddc_sources;
    std::vector<std::string> duc_destinations;
    // Enumerate blocks in the chain
    auto edges = graph->enumerate_static_connections();

    // vector of string identifiers for DDC destinations
    for (auto& rctrl : radio_ctrls) {
        ddc_sources.push_back(rctrl->get_block_id());
        duc_destinations.push_back(rctrl->get_block_id());
    }
    sort(ddc_sources.begin(), ddc_sources.end());
    size_t ddc_source_port = 0;
    size_t duc_dst_port    = 0;
    auto chain             = std::vector<uhd::rfnoc::graph_edge_t>();
    ddc_chan = 0;
    duc_chan = 0;

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
                    ddc_ctrls.push_back(
                        graph->get_block<uhd::rfnoc::ddc_block_control>(
                            blockid));
                }
            }
        }
        for (auto& dst_block : duc_destinations) {
            if (edge.dst_blockid == dst_block && edge.dst_port == duc_dst_port) {
                auto blockid = uhd::rfnoc::block_id_t(edge.src_blockid);
                if (blockid.match("DUC")) {
                    // When a DUC is found, assemble a vector of DUC controllers.
                    duc_ctrls.push_back(
                        graph->get_block<uhd::rfnoc::duc_block_control>(
                            blockid));
                }
            }
        }
        chain.push_back(edge);
    }
    sort(ddc_ctrls.begin(), ddc_ctrls.end());
    sort(duc_ctrls.begin(), duc_ctrls.end());
    // For Display Purposes
    for (auto& dctrl : ddc_ctrls) {
        std::cout << "Using DDC " << dctrl->get_block_id() << ", Channel "
                  << ddc_chan << std::endl;
    }
    // For Display Purposes
    for (auto& dctrl : duc_ctrls) {
        std::cout << "Using DUC " << dctrl->get_block_id() << ", Channel "
                  << duc_chan << std::endl;
    }
}
void ArchUSRP::buildReplay()
{
    /****************************************************************************
     * Seek Replay blocks on each USRP and assemble a vector of Replay Block Controllers
     ***************************************************************************/
    // Check what replay blocks exist on the device(s)
    replay_block_list = graph->find_blocks("Replay");
    for (auto& replay_id : replay_block_list) {
        // The double entry is intended. There are two channels on each replay block.
        replay_ctrls.push_back(
            graph->get_block<uhd::rfnoc::replay_block_control>(replay_id));
        replay_ctrls.push_back(
            graph->get_block<uhd::rfnoc::replay_block_control>(replay_id));
    }
    sort(replay_ctrls.begin(), replay_ctrls.end());
    // Each replay block has two channels, 0 and 1.
    // Vector of replay channels.
    for (int i_chan = 0; i_chan < replay_ctrls.size() * 2; i_chan++) {
        if (i_chan % 2 == 0) {
            replay_chan_vector.push_back(0);
        } else {
            replay_chan_vector.push_back(1);
        }
    }
    // For Display Purposes
    int count_replay_chan = 0;
    for (auto& rctrl : replay_ctrls) {
        std::cout << "Using Replay " << rctrl->get_block_id() << ", Channel "
                  << replay_chan_vector[count_replay_chan] << std::endl;
        count_replay_chan++;
    }
}
void ArchUSRP::buildStreams()
{
    // Build streams for single threaded implementation.
    // Constants related to the Replay block
    const size_t replay_word_size = 8; // Size of words used by replay block
    const size_t sample_size      = 4; // Complex signed 16-bit is 32 bits per sample
    const size_t samples_per_word = 2; // Number of sc16 samples per word
    uhd::device_addr_t streamer_args(streamargs);
    // create a receive streamer
    uhd::stream_args_t stream_args(format, otw);
    stream_args.args = streamer_args;
    std::cout << "Using streamer args: " << stream_args.args.to_string() << std::endl;
    rx_stream = graph->create_rx_streamer(
        radio_ctrls.size(), stream_args);

    /************************************************************************
     * Set up streamer to Replay blocks
     ***********************************************************************/
    for (int i_s2r = 0; i_s2r < replay_ctrls.size(); i_s2r = i_s2r + 2) {
        streamer_args["block_id"] =
            replay_ctrls[i_s2r]->get_block_id().to_string();
        streamer_args["block_port"] = std::to_string(0);
        stream_args.args            = streamer_args;
        stream_args.channels        = {0};

        tx_stream = graph->create_tx_streamer(
            stream_args.channels.size(), stream_args);
        size_t tx_spp = tx_stream->get_max_num_samps();
        // Make sure that stream SPP is a multiple of the Replay Block Word Size
        if (tx_spp % samples_per_word != 0) {
            // Round SPP down to a multiple of the word size
            tx_spp                  = (tx_spp / samples_per_word) * samples_per_word;
            streamer_args["spp"]    = std::to_string(tx_spp);
            stream_args.args        = streamer_args;
            tx_stream = graph->create_tx_streamer(
                stream_args.channels.size(), stream_args);
        }
        // Vector of tx streamers, duplicate for vector length padding.
        tx_stream_vector.push_back(tx_stream);
        tx_stream_vector.push_back(tx_stream);
    }
}
void ArchUSRP::connectGraph()
{
    // This is the connect function for single threaded implementation.
    UHD_LOG_INFO("CogRF", "Connecting graph...");
    // Connect Graph
    for (int i = 0; i < radio_ctrls.size(); i++) {
        // connect radios to ddc
        graph->connect(radio_block_list[i],
            0,
            ddc_ctrls[i]->get_block_id(),
            0);
        std::cout << "Connected " << radio_block_list[i] << " to "
                  << ddc_ctrls[i]->get_block_id() << std::endl;
        graph->connect(
            ddc_ctrls[i]->get_block_id(), 0, rx_stream, i);
        std::cout << "Connected " << ddc_ctrls[i]->get_block_id() << " to "
                  << rx_stream << " Port " << i << std::endl;
    }
    int pos2 = 0;
    if (duc_ctrls.size() > 0) {
        for (auto& rctrl : radio_ctrls) {
            graph->connect(duc_ctrls[pos2]->get_block_id(),
                duc_chan,
                rctrl->get_block_id(),
                0);
            std::cout << "Connected " << duc_ctrls[pos2]->get_block_id()
                      << " port " << duc_chan << " to radio "
                      << rctrl->get_block_id() << " port " << 0 << std::endl;
            pos2++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        for (int i_r2d = 0; i_r2d < duc_ctrls.size(); i_r2d++) {
            // Catch timeout exception issue.
            while (true) {
                try {
                    graph->connect(
                        replay_ctrls[i_r2d]->get_block_id(),
                        replay_chan_vector[i_r2d],
                        duc_ctrls[i_r2d]->get_block_id(),
                        duc_chan);
                    break;
                } catch (...) {
                    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
            std::cout << "Connected " << replay_ctrls[i_r2d]->get_block_id()
                      << " port " << replay_chan_vector[i_r2d] << " to DUC "
                      << duc_ctrls[i_r2d]->get_block_id() << " port "
                      << duc_chan << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    } else {
        int pos2 = 0;
        for (auto& replctrl : replay_ctrls) {
            for (auto& rctrl : radio_ctrls) {
                while (true) {
                    try {
                        graph->connect(replctrl->get_block_id(),
                            replay_chan_vector[pos2],
                            rctrl->get_block_id(),
                            0);
                        break;
                    } catch (...) {
                        // std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    }
                }
                pos2++;
            }
        }
    }
    for (int i_s2r = 0; i_s2r < replay_ctrls.size(); i_s2r = i_s2r + 2) {
        graph->connect(tx_stream_vector[i_s2r],
            0,
            replay_ctrls[i_s2r]->get_block_id(),
            0);
        std::cout << "Streamer: " << tx_stream_vector[i_s2r]
                  << " connected to " << replay_ctrls[i_s2r]->get_block_id()
                  << std::endl;
    }
}
void ArchUSRP::commitGraph()
{
    UHD_LOG_INFO("CogRF", "Committing graph...");
    graph->commit();
    UHD_LOG_INFO("CogRF", "Commit complete.");
}
void ArchUSRP::connectGraphMultithread()
{
    // This is the function that connects the graph for the multithreaded implementation
    // The difference is that each channel gets its own RX streamer.
    UHD_LOG_INFO("CogRF", "Connecting graph...");
    // Connect Graph
    for (int i = 0; i < radio_ctrls.size(); i++) {
        // connect radios to ddc
        graph->connect(radio_block_list[i],
            0,
            ddc_ctrls[i]->get_block_id(),
            0);
        std::cout << "Connected " << radio_block_list[i] << " to "
                  << ddc_ctrls[i]->get_block_id() << std::endl;
    }
    for (int j = 0; j < ddc_ctrls.size(); j++) {
        // Connect DDC to streamers
        // Reusing replay chan vector, need a vector of zeros and ones
        graph->connect(ddc_ctrls[j]->get_block_id(),
            0,
            rx_stream_vector[j],
            replay_chan_vector[j]);
        std::cout << "Connected " << ddc_ctrls[j]->get_block_id() << " to "
                  << rx_stream_vector[j] << " Port "
                  << replay_chan_vector[j] << std::endl;
    }
    int pos2 = 0;
    if (duc_ctrls.size() > 0) {
        for (auto& rctrl : radio_ctrls) {
            graph->connect(duc_ctrls[pos2]->get_block_id(),
                duc_chan,
                rctrl->get_block_id(),
                0);
            std::cout << "Connected " << duc_ctrls[pos2]->get_block_id()
                      << " port " << duc_chan << " to radio "
                      << rctrl->get_block_id() << " port " << 0 << std::endl;
            pos2++;
        }
        for (int i_r2d = 0; i_r2d < duc_ctrls.size(); i_r2d++) {
            graph->connect(
                replay_ctrls[i_r2d]->get_block_id(),
                replay_chan_vector[i_r2d],
                duc_ctrls[i_r2d]->get_block_id(),
                duc_chan);
            std::cout << "Connected " << replay_ctrls[i_r2d]->get_block_id()
                      << " port " << replay_chan_vector[i_r2d] << " to DUC "
                      << duc_ctrls[i_r2d]->get_block_id() << " port "
                      << duc_chan << std::endl;
        }
    } else {
        int pos2 = 0;
        for (auto& replctrl : replay_ctrls) {
            for (auto& rctrl : radio_ctrls) {
                graph->connect(replctrl->get_block_id(),
                    replay_chan_vector[pos2],
                    rctrl->get_block_id(),
                    0);
                pos2++;
            }
        }
    }
    for (int i_s2r = 0; i_s2r < replay_ctrls.size(); i_s2r = i_s2r + 2) {
        graph->connect(tx_stream_vector[i_s2r],
            0,
            replay_ctrls[i_s2r]->get_block_id(),
            0);
        std::cout << "Streamer: " << tx_stream_vector[i_s2r]
                  << " connected to " << replay_ctrls[i_s2r]->get_block_id()
                  << std::endl;
    }
}
void ArchUSRP::buildStreamsMultithread()
{
    // Build Streams for multithreaded implementation
    // Each Channel gets its own RX streamer.
    // Constants related to the Replay block
    const size_t replay_word_size = 8; // Size of words used by replay block
    const size_t sample_size      = 4; // Complex signed 16-bit is 32 bits per sample
    const size_t samples_per_word = 2; // Number of sc16 samples per word
    uhd::device_addr_t streamer_args(streamargs);
    // create a receive streamer
    uhd::stream_args_t stream_args(format, otw); 
    stream_args.args = streamer_args;
    std::cout << "Using streamer args: " << stream_args.args.to_string() << std::endl;
    // One stream per channel
    for (int rx_count = 0; rx_count < radio_ctrls.size() / 2; rx_count++) {
        rx_stream = graph->create_rx_streamer(2, stream_args);
        rx_stream_vector.push_back(rx_stream);
        rx_stream_vector.push_back(rx_stream);
    }
    /************************************************************************
     * Set up streamer to Replay blocks
     ***********************************************************************/
    for (int i_s2r = 0; i_s2r < replay_ctrls.size(); i_s2r = i_s2r + 2) {
        streamer_args["block_id"] =
            replay_ctrls[i_s2r]->get_block_id().to_string();
        streamer_args["block_port"] = std::to_string(0);
        stream_args.args            = streamer_args;
        stream_args.channels        = {0};
        tx_stream = graph->create_tx_streamer(
            stream_args.channels.size(), stream_args);
        size_t tx_spp = tx_stream->get_max_num_samps();
        // Make sure that stream SPP is a multiple of the Replay Block Word Size
        if (tx_spp % samples_per_word != 0) {
            // Round SPP down to a multiple of the word size
            tx_spp                  = (tx_spp / samples_per_word) * samples_per_word;
            streamer_args["spp"]    = std::to_string(tx_spp);
            stream_args.args        = streamer_args;
            tx_stream = graph->create_tx_streamer(
                stream_args.channels.size(), stream_args);
        }
        // Vector of tx streamers, duplicate for vector length padding.
        tx_stream_vector.push_back(tx_stream);
        tx_stream_vector.push_back(tx_stream);
    }
}
//blocksettings
int ArchUSRP::setRadioRates()
{
    /************************************************************************
     * Set up radio, DDCs, and DUCs
     ***********************************************************************/
    // set the RX sample rate
    if (rx_rate <= 0.0) {
        std::cerr << "Please specify a valid RX sample rate" << std::endl;
        return EXIT_FAILURE;
    }
    // set the TX sample rate
    if (tx_rate <= 0.0) {
        std::cerr << "Please specify a valid TX sample rate" << std::endl;
        return EXIT_FAILURE;
    }
    // Set DDC & DUC Sample Rates
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rx_rate / 1e6)
              << std::endl;
    // set rates for each DDC Block
    // TODO: Look at rates here. rate == ....
    // TODO: Fix Display
    if (ddc_ctrls.size() > 0) {
        int count_rx = 0;
        for (uhd::rfnoc::ddc_block_control::sptr ddcctrl : ddc_ctrls) {
            std::cout << "DDC block found" << std::endl;
            double radio_rate = radio_ctrls[count_rx]->get_rate();
            int decim = (int)(radio_rate / rx_rate);
            std::cout << boost::format("Setting decimation value to %d") % decim
                      << std::endl;
            ddcctrl->set_property<int>("decim", decim, ddc_chan);
            decim = ddcctrl->get_property<int>("decim", ddc_chan);
            std::cout << boost::format("Actual decimation value is %d") % decim
                      << std::endl;
            rx_rate = radio_rate / decim;
            count_rx++;
        }
    } else {
        for (auto& rctrl : radio_ctrls) {
            rx_rate = rctrl->set_rate(rx_rate);
            std::cout << boost::format("Actual RX Rate: %f Msps...") % (rx_rate / 1e6)
                      << std::endl
                      << std::endl;
        }
    }
    std::cout << "Actual RX Rate: " << (rx_rate / 1e6) << " Msps..." << std::endl
              << std::endl;
    std::cout << std::resetiosflags(std::ios::fixed);
    std::cout << "Setting TX Rate: " << (tx_rate / 1e6) << " Msps..." << std::endl;
    if (duc_ctrls.size() > 0) {
        int radio = 0;
        for (uhd::rfnoc::duc_block_control::sptr dctrl : duc_ctrls) {
            std::cout << "DUC block found." << dctrl->get_block_id() << std::endl;
            dctrl->set_input_rate(tx_rate, duc_chan);
            dctrl->set_output_rate(
                radio_ctrls[radio]->get_rate(), duc_chan);
            std::cout << dctrl->get_block_id() << " Interpolation value is "
                      << dctrl->get_property<int>("interp", duc_chan)
                      << std::endl;
            tx_rate = dctrl->get_input_rate(duc_chan);
            radio++;
        }
    } else {
        for (uhd::rfnoc::radio_control::sptr rctrl : radio_ctrls) {
            tx_rate = rctrl->set_rate(tx_rate);
        }
    }
    std::cout << "Actual TX Rate: " << (tx_rate / 1e6) << " Msps..." << std::endl
              << std::endl
              << std::resetiosflags(std::ios::fixed);
    return EXIT_SUCCESS;
}
void ArchUSRP::tuneRX()
{
    for (auto& rctrl : radio_ctrls) {
        // Set USRP RX Frequency for All Devices
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting RX Freq: " << std::fixed
                  << (rx_freq / 1e6) << " MHz..." << std::endl;

        rctrl->set_rx_frequency(rx_freq, 0);
        std::cout << rctrl->get_block_id()
                  << " Actual RX Freq: " << (rctrl->get_rx_frequency(0) / 1e6)
                  << " MHz..." << std::endl
                  << std::endl;
    }
}
void ArchUSRP::tuneTX()
{
    for (auto& rctrl : radio_ctrls) {
        // Set USRP TX Frequency for All devices
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting TX Freq: " << std::fixed
                  << (tx_freq / 1e6) << " MHz..." << std::endl;
        rctrl->set_tx_frequency(tx_freq, 0);
        std::cout << rctrl->get_block_id()
                  << " Actual TX Freq: " << (rctrl->get_tx_frequency(0) / 1e6)
                  << " MHz..." << std::endl
                  << std::endl;
    }
}
void ArchUSRP::setRXGain()
{
    // Set RX Gain of all Devices (Appears that max in UHD Is 65)
    for (auto& rctrl : radio_ctrls) {
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting RX Gain: " << rx_gain << " dB..."
                  << std::endl;
        rctrl->set_rx_gain(rx_gain, 0);
        std::cout << rctrl->get_block_id() << " Actual RX Gain: " << rctrl->get_rx_gain(0)
                  << " dB..." << std::endl
                  << std::endl;
        std::cout << std::resetiosflags(std::ios::fixed);
    }
}
void ArchUSRP::setTXGain()
{
    // Set TX Gain of all devices (Appears that max in UHD is 65)
    for (auto& rctrl : radio_ctrls) {
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting TX Gain: " << tx_gain << " dB..."
                  << std::endl;
        rctrl->set_tx_gain(tx_gain, 0);
        std::cout << rctrl->get_block_id() << " Actual TX Gain: " << rctrl->get_tx_gain(0)
                  << " dB..." << std::endl
                  << std::endl;
        std::cout << std::resetiosflags(std::ios::fixed);
    }
}
void ArchUSRP::setRXBw()
{
    // Set RX BandWidth for all devices
    if (rx_bw > 0) {
        for (auto& rctrl : radio_ctrls) {
            std::cout << std::fixed;
            std::cout << rctrl->get_block_id()
                      << " Setting RX Bandwidth: " << (rx_bw / 1e6) << " MHz..."
                      << std::endl;
            rctrl->set_rx_bandwidth(rx_bw, 0);
            std::cout << rctrl->get_block_id()
                      << " Actual RX Bandwidth: " << (rctrl->get_rx_bandwidth(0) / 1e6)
                      << " MHz..." << std::endl
                      << std::endl;
            std::cout << std::resetiosflags(std::ios::fixed);
        }
    }
}
void ArchUSRP::setTXBw()
{
    // Set TX BandWidth for all devices
    if (tx_bw > 0) {
        for (auto& rctrl : radio_ctrls) {
            std::cout << std::fixed;
            std::cout << rctrl->get_block_id()
                      << " Setting TX Bandwidth: " << (tx_bw / 1e6) << " MHz..."
                      << std::endl;
            rctrl->set_tx_bandwidth(tx_bw, 0);
            std::cout << rctrl->get_block_id()
                      << " Actual TX Bandwidth: " << (rctrl->get_tx_bandwidth(0) / 1e6)
                      << " MHz..." << std::endl
                      << std::endl;
            std::cout << std::resetiosflags(std::ios::fixed);
        }
    }
}
void ArchUSRP::setRXAnt()
{
    // Set RX Antenna for all devices
    for (auto& rctrl : radio_ctrls) {
        rctrl->set_rx_antenna(rx_ant, 0);
    }
}
void ArchUSRP::setTXAnt()
{
    // Set TX Antenna for all devices
    for (auto& rctrl : radio_ctrls) {
        rctrl->set_tx_antenna(tx_ant, 0);
    }
}
//recvdata
void ArchUSRP::recv(int rx_channel_nums, int threadnum, uhd::rx_streamer::sptr rx_streamer){
    // Receive to memory only, multi-threaded implementation.
    uhd::set_thread_priority_safe(0.9F);
    int num_total_samps = 0;
    // Prepare buffers for received samples and metadata
    uhd::rx_metadata_t md;
    std::vector<boost::circular_buffer<std::complex<short>>> buffs(
        rx_channel_nums, boost::circular_buffer<std::complex<short>>(spb+1));
    // create a vector of pointers to point to each of the channel buffers
    std::vector<std::complex<short>*> buff_ptrs;
    for (size_t i = 0; i < buffs.size(); i++) {
        buff_ptrs.push_back(&buffs[i].front());
    }
    bool overflow_message = true;
    // setup streaming
    uhd::stream_cmd_t stream_cmd((nsamps == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = nsamps;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = time_spec;
    md.has_time_spec      = true;
    md.time_spec          = time_spec;
    const auto stop_time = time_spec + uhd::time_spec_t(time_requested);
    rx_streamer->issue_stream_cmd(stream_cmd);
    int loop_num =0;
    while (not stop_signal_called
           and (nsamps > num_total_samps or nsamps == 0)
           and (time_requested == 0.0
                or graph->get_mb_controller(0)
                           ->get_timekeeper(0)
                           ->get_time_now()
                       <= stop_time)) {
        size_t num_rx_samps = rx_streamer->recv(buff_ptrs, spb, md, rx_timeout);
        loop_num += 1;
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cout << boost::format("Timeout while streaming") << std::endl;
            break;
        }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
            if (overflow_message) {
                overflow_message = false;
                std::string tempstr ="\n thread:"+std::to_string(threadnum)+'\n'+"loop_num:"+std::to_string(loop_num)+'\n';
                std::cout<<tempstr;
            }
            if(md.out_of_sequence != true){
                std::cerr
                    << boost::format(
                           "Got an overflow indication. Please consider the following:\n"
                           "  Your write medium must sustain a rate of %fMB/s.\n"
                           "  Dropped samples will not be written to the file.\n"
                           "  Please modify this example for your purposes.\n"
                           "  This message will not appear again.\n")
                           % (rx_rate * sizeof(std::complex<short>) / 1e6);
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
void ArchUSRP::passthru(void* arg ,int rx_channel_nums, int threadnum, uhd::rx_streamer::sptr rx_streamer ){
    ArchUSRP* archUsrpPassthru = (ArchUSRP*) arg;
    archUsrpPassthru->recv(rx_channel_nums,threadnum, rx_streamer);
}
