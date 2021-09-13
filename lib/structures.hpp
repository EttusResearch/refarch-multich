#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>

#include <boost/lockfree/spsc_queue.hpp>

#include <iostream>
#include <boost/format.hpp>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <fstream>
#include <mutex>

extern volatile bool stop_signal_called;


//This struct handles reading the config file and parsing the data. 
struct ProgramMetaData
{
    

    //Input argument
    std::string cfgFile;

    //runtime
    boost::program_options::options_description desc;
    boost::program_options::variables_map vm;

    //Avoid boilerplate in main by initializing desc with cfgFile option 
    ProgramMetaData()
    {
        namespace po = boost::program_options;
        // clang-format off
        desc.add_options()
        ("cfgFile", po::value<std::string>(&cfgFile), "relative path to configuration file")
        ;
        // clang-format on
    }

    void storeProgramOptions(int argc, char* argv[])
    {
        namespace po = boost::program_options;

        // store program options from cmd line
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        

        // store program options from config file
        if (vm.count("cfgFile")) {
            std::cout << "Load cfg_file: " << cfgFile << std::endl;
            //standard streams don't accept a standard string, so pass the string using c_str()
            po::store(po::parse_config_file(cfgFile.c_str(), desc), vm);
            po::notify(vm);
        }
        
    }
};







struct SignalSettings{


    //Runtime
    size_t samples_to_replay;

    //Load from disk
    std::string rx_file;
    std::string otw;
    std::string type;
    size_t spb, nruns;
    double rx_timeout; 
    double time_adjust;
    size_t nsamps;
    double rtime; 
    double rep_delay; //replay block time
    std::string format;
    std::string file;
    bool singleTXRX_loopback;
    int singleTX, singleRX;
    double time_requested;
    bool stop_signal_called;
   
   

    void addProgramOptions( boost::program_options::options_description &desc )
    {
    namespace po = boost::program_options;
        // clang-format off
        //TODO: Verify we are still using the comments for each value in
        //or if we can delete and push explaination to top of each structure
        desc.add_options()
        ("rx-file", po::value<std::string>(&rx_file)->default_value("test.dat"), "name of the file to write binary samples to")
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
        ("singleTXRX_loopback", po::value<bool>(&singleTXRX_loopback)->default_value(false), "Loopback between a single TX and single RX channel.")
        ("singleTX",po::value<int>(&singleTX)->default_value(0), "Single Loopback TX Channel)")
        ("singleRX",po::value<int>(&singleRX)->default_value(0), "Single Loopback RX Channel)")
        ("time_requested", po::value<double>(&time_requested)->default_value(0.0), "Single Loopback Continous Time Limit (s).")



        ;
        // clang-format on
    }

    
};

struct DeviceSettings{
    

    //Runtime
    std::string argsWithAddress;
    std::string folder_name;

    //Load from disk
    std::string args;
    double tx_rate, tx_freq, tx_gain,tx_bw;
    double rx_rate, rx_freq, rx_gain, rx_bw;
    std::string ref;
    
    std::string tx_ant, rx_ant;
    std::string streamargs;
    std::vector<std::string> address;
    std::string tx_args;
    std::string rx_args;
    std::vector<std::string> lo;

   

    void addProgramOptions( boost::program_options::options_description &desc )
    {
    namespace po = boost::program_options;
        // clang-format off
        //TODO: Verify we are still using the comments for each value in
        //or if we can delete and push explaination to top of each structure

        desc.add_options()
        ("args", po::value<std::string>(&args)->default_value(""), "uhd transmit device address args")
        ("tx-rate", po::value<double>(&tx_rate), "rate of transmit outgoing samples")
        ("rx-rate", po::value<double>(&rx_rate), "rate of receive incoming samples")
        ("tx-gain", po::value<double>(&tx_gain), "gain for the transmit RF chain")
        ("rx-gain", po::value<double>(&rx_gain), "gain for the receive RF chain")
        ("ref", po::value<std::string>(&ref)->default_value("external"), "clock reference (internal, external, mimo)")
        ("tx-freq", po::value<double>(&tx_freq), "transmit RF center frequency in Hz")
        ("rx-freq", po::value<double>(&rx_freq), "receive RF center frequency in Hz")
        ("address", po::value<std::vector<std::string>>(&address), "uhd transmit device address args")
        ("tx-ant", po::value<std::string>(&tx_ant)->default_value("TX/RX"), "transmit antenna selection")
        ("rx-ant", po::value<std::string>(&rx_ant)->default_value("RX2"), "receive antenna selection")
        ("streamargs", po::value<std::string>(&streamargs)->default_value(""), "stream args")
        ("tx-bw", po::value<double>(&tx_bw), "analog transmit filter bandwidth in Hz")
        ("rx-bw", po::value<double>(&rx_bw), "analog receive filter bandwidth in Hz")
        ("tx-args", po::value<std::string>(&tx_args)->default_value(""), "uhd transmit device address args")
        ("rx-args", po::value<std::string>(&rx_args)->default_value(""), "uhd receive device address args")
        ("lo", po::value<std::vector<std::string>>(&lo), "device LO settings")

        ;
        // clang-format on
    }

    //when loading the args from a configuration file the address is split per 
    //device so add them back to the args
    void addAddresstoArgs()
    {
        argsWithAddress = "" + args;

        for (const auto& addr : address)
        {
            argsWithAddress += ", " + addr;
        }

        args = argsWithAddress;
    }

   


    

};

struct GraphSettings{

    //rfnoc graph
    uhd::rfnoc::rfnoc_graph::sptr graph;
    //radio Global Variables
    std::vector<uhd::rfnoc::radio_control::sptr> radio_ctrls;
    std::vector<uhd::rfnoc::block_id_t> radio_block_list;
    //DDC/DUC Global Variables
    std::vector<uhd::rfnoc::ddc_block_control::sptr> ddc_ctrls;
    std::vector<uhd::rfnoc::duc_block_control::sptr> duc_ctrls;
    size_t ddc_chan;
    size_t duc_chan;
    //Replay Global Variables
    std::vector<uhd::rfnoc::replay_block_control::sptr> replay_ctrls;
    std::vector<size_t> replay_chan_vector;
    std::vector<uhd::rfnoc::block_id_t> replay_block_list;
    uint32_t replay_buff_addr;
    uint32_t replay_buff_size;
    //Streamer Variables
    uhd::rx_streamer::sptr rx_stream;
    uhd::tx_streamer::sptr tx_stream;
    std::vector<size_t> streamer_channels;
    std::vector<uhd::tx_streamer::sptr> tx_stream_vector;
    std::vector<uhd::rx_streamer::sptr> rx_stream_vector; 
    //txrx settings
    uhd::time_spec_t time_spec;
    uhd::time_spec_t time_adjustment;

    
    
    

    

};



#endif