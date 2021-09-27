
#include "blocksettings.hpp"
#include "structures.hpp"
#include "sync.hpp"
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/types/tune_request.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <thread>


int BlockSettings::setRadioRates(
    GraphSettings& graphSettings, double& rx_rate, double& tx_rate)
{
    /************************************************************************
     * Set up radio, DDCs, and DUCs
     ***********************************************************************/
    // set the RX sample rate
    if (rx_rate <= 0.0) {
        std::cerr << "Please specify a valid RX sample rate" << std::endl;
        return EXIT_FAILURE;
    }

    // set the TX sample rate
    if (tx_rate <= 0.0) {
        std::cerr << "Please specify a valid TX sample rate" << std::endl;
        return EXIT_FAILURE;
    }

    // Set DDC & DUC Sample Rates


    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rx_rate / 1e6)
              << std::endl;
    // set rates for each DDC Block
    // TODO: Look at rates here. rate == ....
    // TODO: Fix Display
    if (graphSettings.ddc_ctrls.size() > 0) {
        int count_rx = 0;
        for (uhd::rfnoc::ddc_block_control::sptr ddcctrl : graphSettings.ddc_ctrls) {
            std::cout << "DDC block found" << std::endl;
            double radio_rate = graphSettings.radio_ctrls[count_rx]->get_rate();

            int decim = (int)(radio_rate / rx_rate);
            std::cout << boost::format("Setting decimation value to %d") % decim
                      << std::endl;
            ddcctrl->set_property<int>("decim", decim, graphSettings.ddc_chan);
            decim = ddcctrl->get_property<int>("decim", graphSettings.ddc_chan);
            std::cout << boost::format("Actual decimation value is %d") % decim
                      << std::endl;
            rx_rate = radio_rate / decim;
            count_rx++;
        }
    } else {
        for (auto& rctrl : graphSettings.radio_ctrls) {
            rx_rate = rctrl->set_rate(rx_rate);
            std::cout << boost::format("Actual RX Rate: %f Msps...") % (rx_rate / 1e6)
                      << std::endl
                      << std::endl;
        }
    }

    std::cout << "Actual RX Rate: " << (rx_rate / 1e6) << " Msps..." << std::endl
              << std::endl;
    std::cout << std::resetiosflags(std::ios::fixed);

    std::cout << "Setting TX Rate: " << (tx_rate / 1e6) << " Msps..." << std::endl;
    if (graphSettings.duc_ctrls.size() > 0) {
        int radio = 0;
        for (uhd::rfnoc::duc_block_control::sptr dctrl : graphSettings.duc_ctrls) {
            std::cout << "DUC block found." << dctrl->get_block_id() << std::endl;

            dctrl->set_input_rate(tx_rate, graphSettings.duc_chan);
            dctrl->set_output_rate(
                graphSettings.radio_ctrls[radio]->get_rate(), graphSettings.duc_chan);
            std::cout << dctrl->get_block_id() << " Interpolation value is "
                      << dctrl->get_property<int>("interp", graphSettings.duc_chan)
                      << std::endl;
            tx_rate = dctrl->get_input_rate(graphSettings.duc_chan);
            radio++;
        }


    } else {
        for (uhd::rfnoc::radio_control::sptr rctrl : graphSettings.radio_ctrls) {
            tx_rate = rctrl->set_rate(tx_rate);
        }
    }


    std::cout << "Actual TX Rate: " << (tx_rate / 1e6) << " Msps..." << std::endl
              << std::endl;
    std::cout << std::resetiosflags(std::ios::fixed);

    return EXIT_SUCCESS;
}

void BlockSettings::tuneRX(GraphSettings& graphSettings, const double& rx_freq)
{
    for (auto& rctrl : graphSettings.radio_ctrls) {
        // Set USRP RX Frequency for All Devices
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting RX Freq: " << std::fixed
                  << (rx_freq / 1e6) << " MHz..." << std::endl;

        rctrl->set_rx_frequency(rx_freq, 0);
        std::cout << rctrl->get_block_id()
                  << " Actual RX Freq: " << (rctrl->get_rx_frequency(0) / 1e6)
                  << " MHz..." << std::endl
                  << std::endl;
    }
}

void BlockSettings::tuneTX(GraphSettings& graphSettings, const double& tx_freq)
{
    for (auto& rctrl : graphSettings.radio_ctrls) {
        // Set USRP TX Frequency for All devices
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting TX Freq: " << std::fixed
                  << (tx_freq / 1e6) << " MHz..." << std::endl;
        rctrl->set_tx_frequency(tx_freq, 0);
        std::cout << rctrl->get_block_id()
                  << " Actual TX Freq: " << (rctrl->get_tx_frequency(0) / 1e6)
                  << " MHz..." << std::endl
                  << std::endl;
    }
}

void BlockSettings::setRXGain(GraphSettings& graphSettings, const double& rx_gain)
{
    // Set RX Gain of all Devices (Appears that max in UHD Is 65)
    for (auto& rctrl : graphSettings.radio_ctrls) {
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting RX Gain: " << rx_gain << " dB..."
                  << std::endl;
        rctrl->set_rx_gain(rx_gain, 0);
        std::cout << rctrl->get_block_id() << " Actual RX Gain: " << rctrl->get_rx_gain(0)
                  << " dB..." << std::endl
                  << std::endl;
        std::cout << std::resetiosflags(std::ios::fixed);
    }
}

void BlockSettings::setTXGain(GraphSettings& graphSettings, const double& tx_gain)
{
    // Set TX Gain of all devices (Appears that max in UHD is 65)
    for (auto& rctrl : graphSettings.radio_ctrls) {
        std::cout << std::fixed;
        std::cout << rctrl->get_block_id() << " Setting TX Gain: " << tx_gain << " dB..."
                  << std::endl;
        rctrl->set_tx_gain(tx_gain, 0);
        std::cout << rctrl->get_block_id() << " Actual TX Gain: " << rctrl->get_tx_gain(0)
                  << " dB..." << std::endl
                  << std::endl;
        std::cout << std::resetiosflags(std::ios::fixed);
    }
}

void BlockSettings::setRXBw(GraphSettings& graphSettings, const double& rx_bw)
{
    // Set RX BandWidth for all devices
    if (rx_bw > 0) {
        for (auto& rctrl : graphSettings.radio_ctrls) {
            std::cout << std::fixed;
            std::cout << rctrl->get_block_id()
                      << " Setting RX Bandwidth: " << (rx_bw / 1e6) << " MHz..."
                      << std::endl;
            rctrl->set_rx_bandwidth(rx_bw, 0);
            std::cout << rctrl->get_block_id()
                      << " Actual RX Bandwidth: " << (rctrl->get_rx_bandwidth(0) / 1e6)
                      << " MHz..." << std::endl
                      << std::endl;
            std::cout << std::resetiosflags(std::ios::fixed);
        }
    }
}

void BlockSettings::setTXBw(GraphSettings& graphSettings, const double& tx_bw)
{
    // Set TX BandWidth for all devices
    if (tx_bw > 0) {
        for (auto& rctrl : graphSettings.radio_ctrls) {
            std::cout << std::fixed;
            std::cout << rctrl->get_block_id()
                      << " Setting TX Bandwidth: " << (tx_bw / 1e6) << " MHz..."
                      << std::endl;
            rctrl->set_tx_bandwidth(tx_bw, 0);
            std::cout << rctrl->get_block_id()
                      << " Actual TX Bandwidth: " << (rctrl->get_tx_bandwidth(0) / 1e6)
                      << " MHz..." << std::endl
                      << std::endl;
            std::cout << std::resetiosflags(std::ios::fixed);
        }
    }
}

void BlockSettings::setRXAnt(GraphSettings& graphSettings, const std::string& rx_ant)
{
    // Set RX Antenna for all devices
    for (auto& rctrl : graphSettings.radio_ctrls) {
        rctrl->set_rx_antenna(rx_ant, 0);
    }
}

void BlockSettings::setTXAnt(GraphSettings& graphSettings, const std::string& tx_ant)
{
    // Set TX Antenna for all devices
    for (auto& rctrl : graphSettings.radio_ctrls) {
        rctrl->set_tx_antenna(tx_ant, 0);
    }
}
