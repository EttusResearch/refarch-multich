#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/thread.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include "config.hpp"
#include "usrpSettings.hpp"



int UsrpSettings::applySettings()
{
    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("tx-subdev"))
        usrpSession->set_tx_subdev_spec(txSubdev);
    if (vm.count("rx-subdev"))
        usrpSession->set_rx_subdev_spec(rxSubdev);
        //rx_usrp->set_rx_subdev_spec(rx_subdev);

    // Lock mboard clocks
    if (vm.count("ref")) {
        usrpSession->set_clock_source(ref);
        //rx_usrp->set_clock_source(ref);
        usrpSession->set_time_source(ref);

        std::cout << boost::format("Setting device clock source to %s and time source to %s") % usrpSession->get_clock_source(0) % usrpSession->get_time_source(0) << std::endl;
    }
    
    // reset usrp time to prepare for transmit/receive. 
    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    usrpSession->set_time_unknown_pps(uhd::time_spec_t(0.0));
    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); //set_time_unknown_pps can take up to 2s

    /*
    //set cmd time for .1s in the future
    usrpSession->clear_command_time();
    uhd::time_spec_t temp_set_command_time = usrpSession->get_time_now() + uhd::time_spec_t(settling);
    usrpSession->set_command_time(temp_set_command_time); 
    std::cout << boost::format("Clear command time and set command time to: %s.%s") 
                                % temp_set_command_time.get_full_secs() % temp_set_command_time.get_frac_secs() << std::endl;
    */

    std::cout << boost::format("Using TX Device: %s") % usrpSession->get_pp_string()
              << std::endl;
    //std::cout << boost::format("Using RX Device: %s") % rx_usrp->get_pp_string()
    //          << std::endl;

    // set the transmit sample rate
    if (not vm.count("tx-rate")) {
        std::cerr << "Please specify the transmit sample rate with --tx-rate"
                  << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (txRate / 1e6)
              << std::endl;
    usrpSession->set_tx_rate(txRate);
    std::cout << boost::format("Actual TX Rate: %f Msps...")
                     % (usrpSession->get_tx_rate() / 1e6)
              << std::endl
              << std::endl;

    // set the receive sample rate
    if (not vm.count("rx-rate")) {
        std::cerr << "Please specify the sample rate with --rx-rate" << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rxRate / 1e6)
              << std::endl;
    usrpSession->set_rx_rate(rxRate); //rx_usrp
    std::cout << boost::format("Actual RX Rate: %f Msps...")
                     % (usrpSession->get_rx_rate() / 1e6) //rx_usrp
              << std::endl
              << std::endl;

    // set the transmit center frequency
    if (not vm.count("tx-freq")) {
        std::cerr << "Please specify the transmit center frequency with --tx-freq"
                  << std::endl;
        return ~0;
    }

    for (size_t ch = 0; ch < txChannelNums.size(); ch++) {
        size_t channel = txChannelNums[ch];
        if (txChannelNums.size() > 1) {
            std::cout << "Configuring TX Channel " << channel << std::endl;
        }
        std::cout << boost::format("Setting TX Freq: %f MHz...") % (txFreq / 1e6)
                  << std::endl;
        uhd::tune_request_t tx_tune_request(txFreq);
        if (vm.count("tx-int-n"))
            tx_tune_request.args = uhd::device_addr_t("mode_n=integer");
        usrpSession->set_tx_freq(tx_tune_request, channel);
        std::cout << boost::format("Actual TX Freq: %f MHz...")
                         % (usrpSession->get_tx_freq(channel) / 1e6)
                  << std::endl
                  << std::endl;

        // set the rf gain
        if (vm.count("tx-gain")) {
            std::cout << boost::format("Setting TX Gain: %f dB...") % txGain
                      << std::endl;
            usrpSession->set_tx_gain(txGain, channel);
            std::cout << boost::format("Actual TX Gain: %f dB...")
                             % usrpSession->get_tx_gain(channel)
                      << std::endl
                      << std::endl;
        }

        // set the analog frontend filter bandwidth
        if (vm.count("tx-bw")) {
            std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % txBw
                      << std::endl;
            usrpSession->set_tx_bandwidth(txBw, channel);
            std::cout << boost::format("Actual TX Bandwidth: %f MHz...")
                             % usrpSession->get_tx_bandwidth(channel)
                      << std::endl
                      << std::endl;
        }

        // set the antenna
        if (vm.count("tx-ant"))
            usrpSession->set_tx_antenna(txAnt, channel);
    }

    for (size_t ch = 0; ch < rxChannelNums.size(); ch++) {
        size_t channel = rxChannelNums[ch];
        if (rxChannelNums.size() > 1) {
            std::cout << "Configuring RX Channel " << channel << std::endl;
        }

        // set the receive center frequency
        if (not vm.count("rx-freq")) {
            std::cerr << "Please specify the center frequency with --rx-freq"
                      << std::endl;
            return ~0;
        }
        std::cout << boost::format("Setting RX Freq: %f MHz...") % (rxFreq / 1e6)
                  << std::endl;
        uhd::tune_request_t rx_tune_request(rxFreq);
        if (vm.count("rx-int-n"))
            rx_tune_request.args = uhd::device_addr_t("mode_n=integer");
        usrpSession->set_rx_freq(rx_tune_request, channel); //rx_usrp
        std::cout << boost::format("Actual RX Freq: %f MHz...")
                         % (usrpSession->get_rx_freq(channel) / 1e6) //rx_usrp
                  << std::endl
                  << std::endl;

        // set the receive rf gain
        if (vm.count("rx-gain")) {
            std::cout << boost::format("Setting RX Gain: %f dB...") % rxGain
                      << std::endl;
            usrpSession->set_rx_gain(rxGain, channel); //rx_usrp
            std::cout << boost::format("Actual RX Gain: %f dB...")
                             % usrpSession->get_rx_gain(channel) //rx_usrp
                      << std::endl
                      << std::endl;
        }

        // set the receive analog frontend filter bandwidth
        if (vm.count("rx-bw")) {
            std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (rxBw / 1e6)
                      << std::endl;
            usrpSession->set_rx_bandwidth(rxBw, channel); //rx_usrp
            std::cout << boost::format("Actual RX Bandwidth: %f MHz...")
                             % (usrpSession->get_rx_bandwidth(channel) / 1e6) //rx_usrp
                      << std::endl
                      << std::endl;
        }

        // set the receive antenna
        if (vm.count("rx-ant"))
            usrpSession->set_rx_antenna(rxAnt, channel); //rx_usrp
    }
    return 0;
}

void UsrpSettings::configureLo()
{
    // Export LO if n321 and import the LO for both n321 and n320
    if(enableLo == "n321") 
    {
        std::cout << boost::format("Exporting all TX LO and importing TX LO IN1...") << std::endl;
        usrpSession->set_tx_lo_export_enabled(true, "lo1", 0);
        usrpSession->get_device()->get_tree()->access<bool>("mboards/0/dboards/A/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export").set(true);
        usrpSession->get_device()->get_tree()->access<bool>("mboards/0/dboards/A/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export").set(true);
        usrpSession->get_device()->get_tree()->access<bool>("mboards/0/dboards/A/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export").set(true);
        usrpSession->get_device()->get_tree()->access<bool>("mboards/0/dboards/A/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export").set(true);

        //std::cout << boost::format("Importing RX LO IN1...") << std::endl;
        //rx_usrp->set_rx_lo_export_enabled(true, "lo1", 0);
        //rx_usrp->get_device()->get_tree()->access<bool>("mboards/0/dboards/A/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export").set(true);
        //rx_usrp->get_device()->get_tree()->access<bool>("mboards/0/dboards/A/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export").set(true);
        //rx_usrp->get_device()->get_tree()->access<bool>("mboards/0/dboards/A/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export").set(true);
        //rx_usrp->get_device()->get_tree()->access<bool>("mboards/0/dboards/A/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export").set(true);
    }

    for(auto channel : rxChannelNums)
    {
        // Route the LO to the respective channel for both the N321 and N320
        if(enableLo == "n321" || enableLo == "n320") 
        {
            usrpSession->set_rx_lo_source("external", "lo1", channel); //rx_usrp
            std::cout << boost::format("Setting RX LO source for channel %1%...") % channel << std::endl;
        }
    }

    for(auto channel : txChannelNums)
    {
        // Route the LO to the respective channel for both the N321 and N320
        if(enableLo == "n321" || enableLo == "n320") 
        {
            usrpSession->set_tx_lo_source("external", "lo1", channel);
            std::cout << boost::format("Setting TX LO source for channel %1%...") % channel << std::endl;
        }
    }
}