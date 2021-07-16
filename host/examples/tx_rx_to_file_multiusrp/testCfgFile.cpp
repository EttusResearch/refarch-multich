#include <../lib/config.hpp>
#include <uhd/utils/safe_main.hpp>
#include <iostream>
#include <boost/format.hpp>
#include <string>
#include <vector>


//std::vector<size_t> txChannelNums;
//std::vector<size_t> rxChannelNums;
//size_t step;
//uhd::tx_streamer::sptr txStream;
//uhd::usrp::multi_usrp::sptr usrpSession;

struct ProgramMetaData
{
    void storeProgramOptions(int argc, char* argv[]);

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
        ("cfgFile", po::value<std::string>(&cfgFile)->default_value(""), "relative path to configuration file")
        ;
        // clang-format on
    }   
};



void ProgramMetaData::storeProgramOptions(int argc, char* argv[])
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
};

struct DataWorkerSettings{
    void addProgramOptions( boost::program_options::options_description &desc );
    
    //Load from disk
    std::string dataFile;
    std::string waveType;
    std::string otw;
    double waveFreq; 
    std::string fileType;
    size_t totalNumSamps;
    size_t spb;
    int runs;
    double rxTimeout;
    double txBurstReps;
};

void DataWorkerSettings::addProgramOptions( boost::program_options::options_description &desc )
{
    namespace po = boost::program_options;
    // clang-format off
    desc.add_options()
    ("dataFile", po::value<std::string>(&dataFile)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
    ("waveType", po::value<std::string>(&waveType)->default_value("SINE"), "waveform type (CONST, SQUARE, RAMP, SINE)")
    ("otw", po::value<std::string>(&otw)->default_value("sc16"), "specify the over-the-wire sample mode")
    ("waveFreq", po::value<double>(&waveFreq)->default_value(0), "waveform frequency in Hz")
    ("fileType", po::value<std::string>(&fileType)->default_value("short"), "sample type in file: double, float, or short")
    ("totalNumSamps", po::value<size_t>(&totalNumSamps)->default_value(0), "total number of samples to receive")
    ("spb", po::value<size_t>(&spb)->default_value(0), "samples per buffer, 0 for default")
    ("runs", po::value<int>(&runs)->default_value(int(1)), "number of runs, each run creates a set of data files")
    ("rxTimeout", po::value<double>(&rxTimeout)->default_value(double(3)), "number of seconds before rx streamer times out")
    ("txBurstReps", po::value<double>(&txBurstReps)->default_value(double(1)), "number of wavetable buffers to transmit on tx")
    ;
    // clang-format on
};


struct DeviceSettings{
    
    void addProgramOptions( boost::program_options::options_description &desc );
    void addAddresstoArgs();

    //Runtime
    std::string argsWithAddress;

    //Load from disk
    std::string args;
    std::string txSubdev;
    std::string rxSubdev;
    double settling;
    double txRate;
    double rxRate;
    float ampl;
    double txGain;
    double rxGain;
    std::string ref;
    std::string txChannels;
    std::string rxChannels;
    double txFreq;
    double rxFreq;
    std::vector<std::string> address;
    std::vector<std::string> loDist;
};

void DeviceSettings::addProgramOptions( boost::program_options::options_description &desc )
{
    namespace po = boost::program_options;
     // clang-format off
    desc.add_options()
    ("args", po::value<std::string>(&args)->default_value(""), "uhd transmit device address args")
    ("txSubdev", po::value<std::string>(&txSubdev), "transmit subdevice specification")
    ("rxSubdev", po::value<std::string>(&rxSubdev), "receive subdevice specification")
    ("settling", po::value<double>(&settling)->default_value(double(0.2)), "settling time (seconds) before receiving")
    ("txRate", po::value<double>(&txRate), "rate of transmit outgoing samples")
    ("rxRate", po::value<double>(&rxRate), "rate of receive incoming samples")
    ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of the waveform [0 to 0.7]")
    ("txGain", po::value<double>(&txGain), "gain for the transmit RF chain")
    ("rxGain", po::value<double>(&rxGain), "gain for the receive RF chain")
    ("ref", po::value<std::string>(&ref)->default_value("internal"), "clock reference (internal, external, mimo)")
    ("txChannels", po::value<std::string>(&txChannels)->default_value("0"), "which TX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
    ("rxChannels", po::value<std::string>(&rxChannels)->default_value("0"), "which RX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
    ("txFreq", po::value<double>(&txFreq), "transmit RF center frequency in Hz")
    ("rxFreq", po::value<double>(&rxFreq), "receive RF center frequency in Hz")
    ("address", po::value<std::vector<std::string>>(&address), "uhd transmit device address args")
    ("loDist", po::value<std::vector<std::string>>(&loDist), "Set the LO options (none, terminal, branch, source)")
    ;
    // clang-format on
};

//If loading the args from a configuration file the address is split per 
//device so add them back to the args
void DeviceSettings::addAddresstoArgs()
{
    argsWithAddress = "" + args;

    for (auto addr : address)
    {
        argsWithAddress += ", " + addr;
    }
}



int UHD_SAFE_MAIN(int argc, char* argv[])
{

    ProgramMetaData pmd;
    DeviceSettings device;
    DataWorkerSettings dws;

    //declare settings
    device.addProgramOptions(pmd.desc);
    dws.addProgramOptions(pmd.desc);

    //initialize settings with information from cmd line and config file
    pmd.storeProgramOptions(argc, argv);

    //Storing the arg parameter as a long string is cumbersome. Instead store each device value 
    //as a seperate string and format them together
    device.addAddresstoArgs();

    // finished
    std::cout << std::endl << device.argsWithAddress << std::endl;
    return EXIT_SUCCESS;
}