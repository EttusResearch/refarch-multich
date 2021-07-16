#include "programOptions.hpp"
#include <iostream>
#include <boost/format.hpp>
#include "config.hpp"

void ProgramOptions::addOptions(int argc, char* argv[])
{
    namespace po = boost::program_options;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("cfg-file", po::value<std::string>(&cfgFile)->default_value(""), "relative path to configuration file") 
        ("args", po::value<std::string>(&args)->default_value(""), "uhd transmit device address args")
        ("data-file", po::value<std::string>(&dataFile)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
        ("fileType", po::value<std::string>(&fileType)->default_value("short"), "sample type in file: double, float, or short")
        ("nsamps", po::value<size_t>(&totalNumSamps)->default_value(0), "total number of samples to receive")
        ("settling", po::value<double>(&settling)->default_value(double(0.2)), "settling time (seconds) before receiving")
        ("spb", po::value<size_t>(&spb)->default_value(0), "samples per buffer, 0 for default")
        ("tx-rate", po::value<double>(&txRate), "rate of transmit outgoing samples")
        ("rx-rate", po::value<double>(&rxRate), "rate of receive incoming samples")
        ("tx-freq", po::value<double>(&txFreq), "transmit RF center frequency in Hz")
        ("rx-freq", po::value<double>(&rxFreq), "receive RF center frequency in Hz")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of the waveform [0 to 0.7]")
        ("tx-gain", po::value<double>(&txGain), "gain for the transmit RF chain")
        ("rx-gain", po::value<double>(&rxGain), "gain for the receive RF chain")
        ("tx-ant", po::value<std::string>(&txAnt), "transmit antenna selection")
        ("rx-ant", po::value<std::string>(&rxAnt), "receive antenna selection")
        ("tx-subdev", po::value<std::string>(&txSubdev), "transmit subdevice specification")
        ("rx-subdev", po::value<std::string>(&rxSubdev), "receive subdevice specification")
        ("tx-bw", po::value<double>(&txBw), "analog transmit filter bandwidth in Hz")
        ("rx-bw", po::value<double>(&rxBw), "analog receive filter bandwidth in Hz")
        ("wave-type", po::value<std::string>(&waveType)->default_value("CONST"), "waveform type (CONST, SQUARE, RAMP, SINE)")
        ("wave-freq", po::value<double>(&waveFreq)->default_value(0), "waveform frequency in Hz")
        ("ref", po::value<std::string>(&ref)->default_value("internal"), "clock reference (internal, external, mimo)")
        ("otw", po::value<std::string>(&otw)->default_value("sc16"), "specify the over-the-wire sample mode")
        ("tx-channels", po::value<std::string>(&txChannels)->default_value("0"), "which TX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("rx-channels", po::value<std::string>(&rxChannels)->default_value("0"), "which RX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("tx-int-n", "tune USRP TX with integer-N tuning")
        ("rx-int-n", "tune USRP RX with integer-N tuning")
        ("enable_lo", po::value<std::string>(&enableLo)->default_value("none"), "enable_lo options (none, n320, n321)")
        ("tx_burst_reps", po::value<double>(&txBurstReps)->default_value(double(1)), "number of wavetable buffers to transmit on tx")
        ("runs", po::value<int>(&runs)->default_value(int(1)), "number of runs, each run creates a set of data files")
        ("rx_timeout", po::value<double>(&rxTimeout)->default_value(double(3)), "number of seconds before rx streamer times out")
    ;
    // clang-format on

    // store program options
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    std::cout << "debug cfg_file: " << cfgFile << std::endl;
    if(cfgFile != "")
    {
        std::cout << "debug cfg_file != blank: " << cfgFile << std::endl;
        //standard streams don't accept a standard string, so pass the string using c_str()
        po::store(po::parse_config_file(cfgFile.c_str(), desc), vm);
        po::notify(vm);
    }

    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("CogRF Synchronized Playback and Record %s") % desc << std::endl;
    }
}