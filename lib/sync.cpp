//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "sync.hpp"
#include "structures.hpp"
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <thread>


void SyncDevices::setSources(GraphSettings& graphSettings, const std::string& ref)
{
    // Set clock reference
    std::cout << "Locking motherboard reference/time sources..." << std::endl;
    // Lock mboard clocks

    for (size_t i = 0; i < graphSettings.graph->get_num_mboards(); ++i) {
        graphSettings.graph->get_mb_controller(i)->set_clock_source(ref);
        graphSettings.graph->get_mb_controller(i)->set_time_source(ref);
    }
}


int SyncDevices::syncAllDevices(GraphSettings& graphSettings)
{
    // Synchronize Devices

    bool sync_result;
    const uhd::time_spec_t timeSpec = 0.0;
    sync_result = graphSettings.graph->synchronize_devices(timeSpec, true);
    // wait for one sec to ensure rising edge is found
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Synchronized" << std::endl;
    if (sync_result != true) {
        std::cout << "Unable to Synchronize Devices " << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void SyncDevices::killLOs(
    GraphSettings& graphSettings, const std::vector<std::string>& lo)
{
    std::cout << "Shutting Down LOs" << std::endl;
    int device = 0;
    while (device < lo.size()) {

        if (lo[device] == "source" or lo[device] == "distributor") {
            // TODO: this did not clean up nice. Reformat.
            graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export")).set(false);
            graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export")).set(false);
            graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export")).set(false);
            graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export")).set(false);

            graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export")).set(false);
            graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export")).set(false);
            graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export")).set(false);
            graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export")).set(false);
                    
        }

        device++;
    }
    std::cout << "Shutting Down LOs: Done!" << std::endl;
}

void SyncDevices::setLOsfromConfig(
    GraphSettings& graphSettings, const std::vector<std::string>& lo)
{
    // Set LOs per config from config file


    int device = 0;


    while (device < lo.size()) {
        if (lo[device] == "source") {
            SyncDevices::setSource(device, graphSettings);
        } else if (lo[device] == "terminal") {
            SyncDevices::setTerminal(device, graphSettings);
        } else if (lo[device] == "distributor") {
            SyncDevices::setDistributor(device, graphSettings);
        }


        device++;
    }
}

void SyncDevices::setSource(int device, GraphSettings& graphSettings)
{
    std::cout << "Setting Device# " << device << " Radio# " << device * 2 << ", Radio# "
              << device * 2 + 1 << " to: "
              << "source" << std::endl;
    // Set Device to System LO Source
    // No difference between RX and TX LOs, just used RX.
    // TODO: Hardcoded number of channels per device.


    graphSettings.radio_ctrls[device * 2]->set_tx_lo_export_enabled(false, "lo1", 0);
    graphSettings.radio_ctrls[device * 2 + 1]->set_tx_lo_export_enabled(false, "lo1", 0);

    graphSettings.radio_ctrls[device * 2]->set_rx_lo_export_enabled(true, "lo1", 0);
    graphSettings.radio_ctrls[device * 2 + 1]->set_rx_lo_export_enabled(true, "lo1", 0);

    // TODO: this did not clean up nice. Reformat.
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export")).set(true);

    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export")).set(true);


    graphSettings.radio_ctrls[device * 2]->set_tx_lo_source("external", "lo1", 0);
    graphSettings.radio_ctrls[device * 2 + 1]->set_tx_lo_source("external", "lo1", 0);

    graphSettings.radio_ctrls[device * 2]->set_rx_lo_source("external", "lo1", 0);
    graphSettings.radio_ctrls[device * 2 + 1]->set_rx_lo_source("external", "lo1", 0);
}

void SyncDevices::setTerminal(int device, GraphSettings& graphSettings)
{
    // TODO: Hardcoded number of channels per device.
    std::cout << "Setting Device# " << device << " Radio# " << device * 2 << ", Radio# "
              << device * 2 + 1 << " to: "
              << "terminal" << std::endl;
    graphSettings.radio_ctrls[device * 2]->set_tx_lo_source("external", "lo1", 0);
    graphSettings.radio_ctrls[device * 2 + 1]->set_tx_lo_source("external", "lo1", 0);

    graphSettings.radio_ctrls[device * 2]->set_rx_lo_source("external", "lo1", 0);
    graphSettings.radio_ctrls[device * 2 + 1]->set_rx_lo_source("external", "lo1", 0);
}

void SyncDevices::setDistributor(int device, GraphSettings& graphSettings)
{
    // TODO: Hardcoded number of channels per device.
    std::cout << "Setting Device# " << device << " Radio# " << device * 2 << ", Radio# "
              << device * 2 + 1 << " to: "
              << "distributor" << std::endl;

    graphSettings.radio_ctrls[device * 2]->set_tx_lo_source("external", "lo1", 0);
    graphSettings.radio_ctrls[device * 2 + 1]->set_tx_lo_source("external", "lo1", 0);

    graphSettings.radio_ctrls[device * 2]->set_rx_lo_source("external", "lo1", 0);
    graphSettings.radio_ctrls[device * 2 + 1]->set_rx_lo_source("external", "lo1", 0);

    graphSettings.radio_ctrls[device * 2]->set_tx_lo_export_enabled(false, "lo1", 0);
    graphSettings.radio_ctrls[device * 2 + 1]->set_tx_lo_export_enabled(false, "lo1", 0);

    graphSettings.radio_ctrls[device * 2]->set_rx_lo_export_enabled(false, "lo1", 0);
    graphSettings.radio_ctrls[device * 2 + 1]->set_rx_lo_export_enabled(false, "lo1", 0);

    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/tx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export")).set(true);

    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_0/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_1/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_2/export")).set(true);
    graphSettings.graph->get_tree()->access<bool>(str(boost::format("%s%d%s") % "blocks/" % device % "/Radio#0/dboard/rx_frontends/0/los/lo1/lo_distribution/LO_OUT_3/export")).set(true);

}

void SyncDevices::checkRXSensorLock(GraphSettings& graphSettings)
{
    // Check Locked RX Sensors

    std::vector<std::string> rx_sensor_names;


    for (auto& rctrl : graphSettings.radio_ctrls) {
        rx_sensor_names = rctrl->get_rx_sensor_names(0);


        for (auto& name : rx_sensor_names) {
            uhd::sensor_value_t rx_sensor_value = rctrl->get_rx_sensor(name, 0);
            std::cout << "Checking RX LO Lock: " << rx_sensor_value.to_pp_string()
                      << std::endl;
            while (rx_sensor_value.to_pp_string() != "all_los: locked") {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "RX LO LOCKED" << std::endl;
        }
    }
}

void SyncDevices::checkTXSensorLock(GraphSettings& graphSettings)
{
    // Check Locked TX Sensors

    std::vector<std::string> tx_sensor_names;

    for (auto& rctrl : graphSettings.radio_ctrls) {
        tx_sensor_names = rctrl->get_tx_sensor_names(0);

        for (auto& name : tx_sensor_names) {
            uhd::sensor_value_t tx_sensor_value = rctrl->get_tx_sensor(name, 0);
            std::cout << "Checking TX LO Lock: " << tx_sensor_value.to_pp_string()
                      << std::endl;
            while (tx_sensor_value.to_pp_string() != "all_los: locked") {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "TX LO LOCKED" << std::endl;
        }
    }
}
