#ifndef ARCHUSRP_H
#define ARCHUSRP_H

#include <uhd/rfnoc_graph.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <boost/program_options.hpp>

//TODO: Need to rethink how to control the stop_signal


class ArchUSRP {
public:
    ArchUSRP(int argc, char* argv[]);


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
    void static sig_int_handler(int); //This has to be a void static. Can't override

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
    void static passthru(void* arg, int rx_channel_nums, int threadnum, 
        uhd::rx_streamer::sptr rx_streamer);

    //Example
    void virtual example() =0;

    void virtual recv(int rx_channel_nums, int threadnum, uhd::rx_streamer::sptr rx_streamer);
    bool static stop_signal_called; 

protected:
///////////////////////////
//////Structures.hpp///////
///////////////////////////
    /////////////////
    //GraphSettings//
    /////////////////
    uhd::rfnoc::rfnoc_graph::sptr graph;
    // radio Global Variables
    std::vector<uhd::rfnoc::radio_control::sptr> radio_ctrls;
    std::vector<uhd::rfnoc::block_id_t> radio_block_list;
    // DDC/DUC Global Variables
    std::vector<uhd::rfnoc::ddc_block_control::sptr> ddc_ctrls;
    std::vector<uhd::rfnoc::duc_block_control::sptr> duc_ctrls;
    size_t ddc_chan;
    size_t duc_chan;
    // Replay Global Variables
    std::vector<uhd::rfnoc::replay_block_control::sptr> replay_ctrls;
    std::vector<size_t> replay_chan_vector;
    std::vector<uhd::rfnoc::block_id_t> replay_block_list;
    uint32_t replay_buff_addr;
    uint32_t replay_buff_size;
    // Streamer Variables
    uhd::rx_streamer::sptr rx_stream;
    uhd::tx_streamer::sptr tx_stream;
    std::vector<size_t> streamer_channels;
    std::vector<uhd::tx_streamer::sptr> tx_stream_vector;
    std::vector<uhd::rx_streamer::sptr> rx_stream_vector;
    // txrx settings
    uhd::time_spec_t time_spec;
    uhd::time_spec_t time_adjustment;

    //////////////////
    //DeviceSettings//
    //////////////////
    // Runtime
    std::string argsWithAddress;
    std::string folder_name;

    // Load from disk
    std::string args;
    double tx_rate, tx_freq, tx_gain, tx_bw;
    double rx_rate, rx_freq, rx_gain, rx_bw;
    std::string ref;

    std::string tx_ant, rx_ant;
    std::string streamargs;
    std::vector<std::string> address;
    std::vector<std::string> lo;

    //////////////////
    //SignalSettings//
    //////////////////
    // Runtime
    size_t samples_to_replay;

    // Load from disk
    std::string rx_file;
    std::vector<std::string> rx_file_location;
    std::vector<std::string> rx_file_streamers;
    std::string otw;
    std::string type;
    size_t spb, nruns;
    double rx_timeout;
    double time_adjust;
    size_t nsamps;
    double rtime;
    double rep_delay; // replay block time
    std::string format;
    std::string file;
    int singleTX;
    double time_requested;

    //////////////////
    //ProgramMetaData//
    //////////////////
    // Input argument
    std::string cfgFile;

    // runtime
    boost::program_options::options_description desc;
    boost::program_options::variables_map vm;

    //Structures
    void virtual addProgramOptions(); //todo: find way of adding additional variables
    void virtual addAddresstoArgs();
    void virtual storeProgramOptions(int argc, char* argv[]);

private:
    void setSource(int device);
    void setTerminal(int device);
    void setDistributor(int device);

    std::map<int,std::string> getStreamerFileLocation(
        std::vector<std::string> rx_file_streamers,
        std::vector<std::string> rx_file_location);
};

#endif
