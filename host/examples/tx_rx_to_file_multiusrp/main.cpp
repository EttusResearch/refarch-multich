#include "createSession.hpp"
#include "detectChannels.hpp"
#include "usrpSettings.hpp"
#include "applicationSettings.hpp"
#include "playbackRecord.hpp"
#include "programOptions.hpp"
#include "checkClocks.hpp"
#include "config.hpp"
#include <uhd/utils/safe_main.hpp>
#include <iostream>
#include "multi_usrp_sync_txrx_tofile.hpp"

/***********************************************************************
 * Main function
 **********************************************************************/

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    ProgramOptions::addOptions(argc, argv);

    CreateSession createSession;
    createSession.multiUsrp();
    
    DetectChannels::detect();

    if (UsrpSettings::applySettings()!=0)
        return ~0;
    UsrpSettings::configureLo();

    CheckClocks::checkAll();

    ApplicationSettings::configure();

    PlaybackRecord::startStreamer();

    // finished
    std::cout << std::endl << "Done!" << std::endl;
    return EXIT_SUCCESS;
}


int UHD_SAFE_MAIN(int argc, char* argv[])
{

    ProgramMetaData pmd;
    DeviceSettings device(pmd.desc);
    DataWorkerSettings dws(pmd.desc);

    //initialize settings with information from cmd line and config file
    pmd.storeProgramOptions(argc, argv);

    //Storing the arg parameter as a long string is cumbersome. Instead store each device value 
    //as a seperate string and format them together
    device.addAddresstoArgs();

    // finished
    std::cout << std::endl << device.argsWithAddress << std::endl;
    return EXIT_SUCCESS;
}