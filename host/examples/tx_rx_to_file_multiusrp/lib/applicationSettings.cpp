#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <csignal>
#include <fstream>
#include <chrono>
#include <thread>
#include <iostream>
#include "waveTable.hpp"
#include "applicationSettings.hpp"
#include "config.hpp"

void ApplicationSettings::configure()
{
    // for the const wave, set the wave freq for small samples per period
    if (waveFreq == 0 and waveType == "CONST") {
        waveFreq = usrpSession->get_tx_rate() / 2;
    }

    // error when the waveform is not possible to generate
    if (std::abs(waveFreq) > usrpSession->get_tx_rate() / 2) {
        throw std::runtime_error("wave freq out of Nyquist zone");
    }
    if (usrpSession->get_tx_rate() / std::abs(waveFreq) > wave_table_len / 2) {
        throw std::runtime_error("wave freq too small for table");
    }

    // pre-compute the waveform values
    step = boost::math::iround(waveFreq / usrpSession->get_tx_rate() * wave_table_len);

    // Allow time for command time tuning to take affect
    std::this_thread::sleep_for(std::chrono::milliseconds(int(settling)*1000));
    //usrpSession->clear_command_time();

    // create a transmit streamer
    // linearly map channels (index0 = channel0, index1 = channel1, ...)
    uhd::stream_args_t stream_args("fc32", otw);
    stream_args.channels             = txChannelNums;
    txStream = usrpSession->get_tx_stream(stream_args);
}