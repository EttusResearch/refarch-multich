#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>
#include "config.hpp"
#include "detectChannels.hpp"

void DetectChannels::detect()
{
    // detect which channels to use
    std::vector<std::string> tx_channel_strings;
    boost::split(tx_channel_strings, txChannels, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < tx_channel_strings.size(); ch++) {
        size_t chan = std::stoi(tx_channel_strings[ch]);
        if (chan >= usrpSession->get_tx_num_channels()) {
            throw std::runtime_error("Invalid TX channel(s) specified.");
        } else
            txChannelNums.push_back(std::stoi(tx_channel_strings[ch]));
    }
    std::vector<std::string> rx_channel_strings;
    boost::split(rx_channel_strings, rxChannels, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < rx_channel_strings.size(); ch++) {
        size_t chan = std::stoi(rx_channel_strings[ch]);
        if (chan >= usrpSession->get_rx_num_channels()) {
        //if (chan >= rx_usrp->get_rx_num_channels()) {
            throw std::runtime_error("Invalid RX channel(s) specified.");
        } else
            rxChannelNums.push_back(std::stoi(rx_channel_strings[ch]));
    }
}