#include <iostream>
#include <boost/format.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include "config.hpp"
#include "createSession.hpp"

void CreateSession::multiUsrp()
{
    std::cout << "debug args: " << args << std::endl;

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the transmit/receive usrp device with: %s...") % args
              << std::endl;
    usrpSession = uhd::usrp::multi_usrp::make(args);
}