#ifndef ARCHUSRP_H
#define ARCHUSRP_H

#include <uhd/rfnoc_graph.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <boost/program_options.hpp>

//TODO: Need to rethink how to control the stop_signal

class RefArch {
public:
    RefArch(int argc, char* argv[]);


    //SyncDevices
    void virtual setSources();
    int virtual syncAllDevices();
    void virtual killLOs();
    void virtual setLOsfromConfig();
    void virtual checkRXSensorLock();
    void virtual checkTXSensorLock();

    //Replaycontrol
    int virtual importData();
    void virtual stopReplay();
    void static sigIntHandler(int); //This has to be a void static. Can't override

    //recievefunctions
    std::string virtual generateRxFilename(const std::string& base_fn,
        const size_t& rx_chan_num,
        const int& tx_chan_num,
        const int& run_num,
        const double& tx_freq,
        const std::string folder_name,
        const std::vector<std::string> rx_streamer_string,
        const std::vector<std::string> rx_file_location);

    //GraphAssembly
    void virtual buildGraph();
    void virtual buildRadios();
    void virtual buildDDCDUC();
    void virtual buildReplay();
    void virtual buildStreams();
    void virtual connectGraph();
    void virtual commitGraph();
    void virtual connectGraphMultithread();
    void virtual buildStreamsMultithread();

    //blocksettings
    int virtual setRadioRates();
    void virtual tuneRX();
    void virtual tuneTX();
    void virtual setRXGain();
    void virtual setTXGain();
    void virtual setRXBw();
    void virtual setTXBw();
    void virtual setRXAnt();
    void virtual setTXAnt();

    //Spawns threads and transmitter
    void virtual spawnReceiveThreads();

    void virtual recv(int rx_channel_nums, int threadnum, uhd::rx_streamer::sptr rx_streamer);
    bool static RA_stop_signal_called; 

    //Example Specific Values
    //These values should be moved when we transition to
    //example modifiable configuration files.

    // Replay Global Variables
    std::vector<uhd::rfnoc::replay_block_control::sptr> RA_replay_ctrls;
    std::vector<size_t> RA_replay_chan_vector;
    std::vector<uhd::rfnoc::block_id_t> RA_replay_block_list;
    uint32_t RA_replay_buff_addr;
    uint32_t RA_replay_buff_size;
    size_t RA_samples_to_replay;

    //These are used in all examples
    int RA_singleTX;
    double RA_delay_start_time;
    //Iterative Loopback
    double RA_time_adjust;
    double RA_rep_delay; // replay block time
    size_t RA_nruns;


protected:
///////////////////////////
//////Structures.hpp///////
///////////////////////////
    /////////////////
    //GraphSettings//
    /////////////////
    uhd::rfnoc::rfnoc_graph::sptr RA_graph;
    // radio Global Variables
    std::vector<uhd::rfnoc::radio_control::sptr> RA_radio_ctrls;
    std::vector<uhd::rfnoc::block_id_t> RA_radio_block_list;
    // DDC/DUC Global Variables
    std::vector<uhd::rfnoc::ddc_block_control::sptr> RA_ddc_ctrls;
    std::vector<uhd::rfnoc::duc_block_control::sptr> RA_duc_ctrls;
    size_t RA_ddc_chan;
    size_t RA_duc_chan;

    // Streamer Variables
    uhd::rx_streamer::sptr RA_rx_stream;
    uhd::tx_streamer::sptr RA_tx_stream;
    std::vector<uhd::tx_streamer::sptr> RA_tx_stream_vector;
    std::vector<uhd::rx_streamer::sptr> RA_rx_stream_vector;
    // txrx settings
    uhd::time_spec_t RA_start_time;

    //////////////////
    //DeviceSettings//
    //////////////////
    // Runtime
    std::string RA_argsWithAddress;

    // Load from disk
    std::string RA_args;
    double RA_tx_rate, RA_tx_freq, RA_tx_gain, RA_tx_bw;
    double RA_rx_rate, RA_rx_freq, RA_rx_gain, RA_rx_bw;
    std::string RA_ref;
    std::string RA_tx_ant, RA_rx_ant;
    std::string RA_streamargs;
    std::vector<std::string> RA_address;
    std::vector<std::string> RA_lo;

    //////////////////
    //SignalSettings//
    //////////////////

    // Load from disk
    std::string RA_rx_file;
    std::vector<std::string> RA_rx_file_location;
    std::vector<std::string> RA_rx_file_channels;
    std::string RA_otw;
    std::string RA_type;
    size_t RA_spb;
    double RA_rx_timeout;
    size_t RA_nsamps;
    std::string RA_format;
    std::string RA_file;
    double RA_time_requested;

    //////////////////
    //ProgramMetaData//
    //////////////////
    // Input argument
    std::string RA_cfgFile;

    // runtime
    boost::program_options::options_description RA_desc;
    boost::program_options::variables_map RA_vm;

    //Structures
    void virtual addProgramOptions(); //todo: find way of adding additional variables
    void virtual addAddresstoArgs();
    void virtual storeProgramOptions(int argc, char* argv[]);

private:
    void setSource(int device);
    void setTerminal(int device);
    void setDistributor(int device);

    std::map<int,std::string> getStreamerFileLocation(
        std::vector<std::string> RA_rx_file_channels,
        std::vector<std::string> RA_rx_file_location);
};

#endif
