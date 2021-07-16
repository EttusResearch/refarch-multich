#ifndef HEADER_H
#define HEADER_H
#include <string>
#include <vector>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include "waveTable.hpp"
#include <uhd/stream.hpp>


// transmit variables to be set by po
extern std::string args, waveType, txAnt, txSubdev, ref, otw, txChannels;
extern double txRate, txFreq, txGain, waveFreq, txBw;
extern float ampl;

// receive variables to be set by po
extern std::string dataFile, fileType, rxAnt, rxSubdev, rxChannels, enableLo;
extern size_t totalNumSamps, spb;
extern double rxRate, rxFreq, rxGain, rxBw, settling, rxTimeout, txBurstReps;
extern int runs;

// programOptions
extern boost::program_options::variables_map vm;
extern std::string cfgFile;

// create session
extern uhd::usrp::multi_usrp::sptr usrpSession;

//detectChannels
extern std::vector<size_t> txChannelNums;
extern std::vector<size_t> rxChannelNums;

//application settings
extern size_t step;
extern uhd::tx_streamer::sptr txStream;

#endif