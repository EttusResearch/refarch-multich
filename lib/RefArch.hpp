#ifndef REFARCH_H
#define REFARCH_H

#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <signal.h>
#include <stdlib.h>
#include <boost/program_options.hpp>
#include <thread>

// TODO: Need to rethink how to control the stop_signal

class RefArch
{
public:
    RefArch(int argc, char* argv[]) : RA_argc(argc), RA_argv(argv){};
    /**
     * @brief For adding custom additional Options to the configuration file
     * See RefArch::addProgramOptions() for default variables
     * Use the following code to generate additional variables.
     *
     *      namespace po = boost::program_options;
     *      RA_desc.add_options()
     *          ("cfgFile",
     *          po::value<std::string>(&RA_cfgFile),
     *          "relative path to configuration file")
     */
    virtual void addAdditionalOptions(){/*NOP*/};

    // SyncDevices
    /**
     * @brief Controls the initialization of the 10MHz and PPS
     */
    virtual void setSources();
    /**
     * @brief Sets the next PPS edge as time 0 on all devies
     * @return int Returns 0 for success and 1 for failure.
     */
    virtual int syncAllDevices();
    /**
     * @brief Shuts down the LOs on all N321s.
     */
    virtual void killLOs();
    /**
     * @brief Uses the parsed configuration properties to
     * configure the LO sources.
     * @details all LO sources to External
     * Distributor- TX/RX export = False
     * Source-      TX export = False
     *              RX export = True (LO source)
     */
    virtual void setLOsfromConfig();
    /**
     * @brief Returns after all sensors are locked
     */
    virtual void checkRXSensorLock();
    /**
     * @brief Returns after all sensors are locked
     */
    virtual void checkTXSensorLock();
    /**
     * @brief Gets the current time on controller 0 and sets
     * RA_start_time to that time plus RA_delay_start_time.
     */
    virtual void updateDelayedStartTime();
    /**
     * @brief Uses the #RA_file waveform to fill all the replayblocks
     *
     * @details the #RA_file then configures the replay block
     * Sends the data to the replay block
     * @return int Returns early if we are unable to fill all replayblocks
     */
    virtual int importData();
    /**
     * @brief Stops all replay blocks
     */
    virtual void stopReplay();
    /**
     * @brief Used to interrupt all threads.
     */
    static void sigIntHandler(int); // This has to be a void static. Can't override

    /**
     * @brief Used for a few examples to write to file. This function
     * can be removed in later releases. Please use with caution.
     *
     * @param base_fn
     * @param rx_chan_num
     * @param tx_chan_num
     * @param run_num
     * @param tx_freq
     * @param folder_name
     * @param rx_streamer_string
     * @param rx_file_location
     * @return std::string
     */
    virtual std::string generateRxFilename(const std::string& base_fn,
        const size_t rx_chan_num,
        const int tx_chan_num,
        const int run_num,
        const double tx_freq,
        const std::string& folder_name,
        const std::vector<std::string>& rx_streamer_string,
        const std::vector<std::string>& rx_file_location);
    /**
     * @brief Create the USRP sessions
     *
     */
    virtual void buildGraph();
    /**
     * @brief Seek Radio Blocks on each USRP and assemble a vector of radio
     * controllers.
     */
    virtual void buildRadios();
    /**
     * @brief Seek DDCs & DUCs on each USRP and assemble a vector of DDC & DUC
     * controllers.
     */
    virtual void buildDDCDUC();
    /**
     * @brief Seek Replay Blocks on each USRP and assemble a vector of Replay Block
     * Controllers
     */
    virtual void buildReplay();
    virtual void commitGraph();
    /**
     * @brief Connects Replay Block to TX
     *  Connects RX Streamer <- (2)DDC <- (2)RX channels
     *  Connects Replay block -> DUC -> TX channel
     *
     * @details If 0 #RA_duc_ctrls then Replay Block -> TX channel directly
     */
    virtual void connectGraphMultithread();
    /**
     * @brief Connects DDC/DUC to streamers
     *  Connects RX Streamer <- (2)DDC <- (2)RX channels
     *  connects TX Streamer -> DUC -> TX Channel
     */
    virtual void connectGraphMultithreadHostTX();
    /**
     * @brief Builds 1 RX streamer per 2 RX channels.
     *  Builds a TX streamer for a Replay Block.
     *
     * @details Both the TX and RX vectors are padded (duplicate)
     */
    virtual void buildStreamsMultithread();
    /**
     * @brief Builds 1 RX streamer per 2 RX channels.
     *  Builds 1 TX streamer per 1 TX channel
     *
     * @details RX vector is padded (duplicate)
     */
    virtual void buildStreamsMultithreadHostTX();
    /**
     * @brief Set the Radio Rates to #RA_rx_rate and #RA_TX_rate
     *
     * @return int 0 on success. 1 on Failure
     */
    virtual int setRadioRates();
    /**
     * @brief Set USRP RX Frequency of #RA_rx_freq for All Devices
     */
    virtual void tuneRX();
    /**
     * @brief Set USRP TX Frequency of #RA_tx_freq for All Devices
     */
    virtual void tuneTX();
    /**
     * @brief Set RX Gain of #RA_rx_gain on all Devices
     */
    virtual void setRXGain();
    /**
     * @brief Set TX Gain of #RA_tx_gain on all Devices
     */
    virtual void setTXGain();
    /**
     * @brief Set RX BandWidth of #RA_rx_bw for all devices
     */
    virtual void setRXBw();
    /**
     * @brief Set TX BandWidth of #RA_tx_bw for all devices
     */
    virtual void setTXBw();
    /**
     * @brief Set RX Antenna of #RA_rx_ant for all devices
     */
    virtual void setRXAnt();
    /**
     * @brief Set TX Antenna of #RA_tx_ant for all devices
     */
    virtual void setTXAnt();
    /**
     * @brief Parses the configuration file for default parameters
     * and user defined variables. Converts address arguments to
     * usable format.
     */
    virtual void parseConfig();
    /**
     * @brief Spawns #RA_rx_stream_vector size number of threads calling RefArch::recv()
     * An override of the recv function will result in this spawning instances of that
     * function.
     */
    virtual void spawnReceiveThreads();
    /**
     * @brief Spawns either a single TX thread or multiple depending on #RA_TX_All_Chan
     *  In either case an override of RefArch::transmitFromFile() will result in the
     *  new function being called.
     */
    virtual void spawnTransmitThreads();
    /**
     * @brief Main loop to acquire samples. Typically all examples override this function
     *
     * @param rx_channel_nums number of channels per streamer
     * @param threadnum thread number
     * @param rx_streamer
     */
    virtual void recv(const int rx_channel_nums,
        const int threadnum,
        uhd::rx_streamer::sptr rx_streamer);
    /**
     * @brief Main loop to stream samples from host. Typically streaming examples
     *  will override this function
     *
     * @param tx_streamer
     * @param metadata meta data that was created from RefArch::spawnTransmitThreads()
     * @param num_channels number of channels per streamer
     */
    void virtual transmitFromFile(uhd::tx_streamer::sptr tx_streamer,
        uhd::tx_metadata_t metadata,
        int num_channels);
    /**
     * @brief Starts transmitting at #RA_start_time and will continue until either
     *  #RA_nsamps or until RefArch::stopReplay() is called.
     */
    virtual void transmitFromReplay();
    /**
     * @brief Waits until it is able to join all Rx and Tx threads.
     */
    virtual void joinAllThreads();
    /**
     * @brief Used to shutdown all threads.
     */
    static bool RA_stop_signal_called;
    std::vector<std::thread> RA_rx_vector_thread;
    std::vector<std::thread> RA_tx_vector_thread;

    // Example Specific Values
    // These values should be moved when we transition to
    // example modifiable configuration files.

    // Replay Global Variables
    std::vector<uhd::rfnoc::replay_block_control::sptr> RA_replay_ctrls;
    std::vector<size_t> RA_replay_chan_vector;
    std::vector<uhd::rfnoc::block_id_t> RA_replay_block_list;
    uint32_t RA_replay_buff_addr;
    uint32_t RA_replay_buff_size;
    size_t RA_samples_to_replay;

    // These are used in all examples
    int RA_singleTX;
    double RA_delay_start_time;
    bool RA_TX_All_Chan;

protected:
    ///////////////////////////
    //////Structures.hpp///////
    ///////////////////////////
    /////////////////
    // GraphSettings//
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
    /**
     * @brief Holds each channels streamer. By default each USRP channel 0 and 1
     *  use the same streamer. RA_rx_stream_vector[0] == RA_rx_stream_vector[1]
     */
    std::vector<uhd::rx_streamer::sptr> RA_rx_stream_vector;
    std::vector<size_t> RA_rx_stream_chan_vector;
    // txrx settings
    uhd::time_spec_t RA_start_time;

    //////////////////
    // DeviceSettings//
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
    // SignalSettings//
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
    /**
     * @brief specifies the input waveform for the TX
     */
    std::string RA_file;
    double RA_time_requested;
    std::string RA_tx_file;

    //////////////////
    // ProgramMetaData//
    //////////////////
    // Input argument
    std::string RA_cfgFile;

    // runtime
    boost::program_options::options_description RA_desc;
    boost::program_options::variables_map RA_vm;
    int RA_argc;
    char** RA_argv;
    // Structures
    /**
     * @brief Default Configuration file variables
     */
    void addProgramOptions();
    void addAddressToArgs();
    void storeProgramOptions();

private:
    void setSource(int device);
    void setTerminal(int device);
    void setDistributor(int device);

    std::map<int, std::string> getStreamerFileLocation(
        const std::vector<std::string>& RA_rx_file_channels,
        const std::vector<std::string>& RA_rx_file_location);
};

#endif
