//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "receivefunctions.hpp"
#include "structures.hpp"
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/exception.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>
#include <csignal>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

// This function generates output file names for the received data.

std::map<int,std::string> getStreamerFileLocation(
    const std::vector<std::string>& rx_file_channels,
    const std::vector<std::string>& rx_file_location){
    std::map<int,std::string> usrpFileMap;
    for(int i =0; i< rx_file_channels.size(); i++){
        size_t pos = 0;
        int n;
        std::string usrpStringArray = rx_file_channels[i];
        while (1) {
            try {
                int n = std::stoi(usrpStringArray,&pos);
                usrpFileMap[n]=rx_file_location[i];
                usrpStringArray = usrpStringArray.substr(pos);
            }
            catch (...) {break;}
        }
    }
    return usrpFileMap;
}

std::string ReceiveFunctions::generateRxFilename(const std::string& base_fn,
    const size_t& rx_chan_num,
    const int& tx_chan_num,
    const int& run_num,
    const double& tx_freq,
    const std::string& folder_name,
    const std::vector<std::string>& rx_streamer_string,
    const std::vector<std::string>& rx_file_location)
{
    // Generates filenames for multithreaded implementation.
    if (rx_file_location.size() != rx_streamer_string.size()){
        throw std::runtime_error(
            "The number of Streamers file locations must match the number of Folder Names");
    }
    std::map<int,std::string> streamer_files = getStreamerFileLocation(rx_streamer_string, rx_file_location);
    boost::filesystem::path base_fn_fp;
    try{
    std::string cw_folder =
        "CW_" + std::to_string(tx_freq * 1e-9) + "_GHz_" + folder_name;
    boost::filesystem::create_directory(
        str(boost::format("%s%s") % streamer_files.at(rx_chan_num) % cw_folder));
    boost::filesystem::path base_fn_fp(streamer_files.at(rx_chan_num) + cw_folder + "/" + base_fn);
    base_fn_fp.replace_extension(boost::filesystem::path(
        str(boost::format("%s%02d%s%02d%s%02d%s%02d%s") % "tx_" % tx_chan_num
            % "_rx_" % rx_chan_num % "_run_" % run_num % "_cw_" % tx_freq 
            % base_fn_fp.extension().string())));
    return base_fn_fp.string();
    }
    catch (const std::out_of_range& oor) {
        throw uhd::runtime_error("One or more file locations were not specified for initialized channel.");
    }
}
