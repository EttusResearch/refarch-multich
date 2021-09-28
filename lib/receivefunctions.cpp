#include "receivefunctions.hpp"
#include "structures.hpp"
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc_graph.hpp>
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


std::string ReceiveFunctions::generateOutFilename(const std::string& base_fn,
    const size_t& rx_chan_num,
    const int& tx_chan_num,
    const int& run_num,
    const double& tx_freq,
    const std::string& folder_name)
{
    // Generates file names for single threaded implementation.
    // Todo: This is not dynamic file creation!

    std::string cw_folder =
        "CW_" + std::to_string(tx_freq * 1e-9) + "_GHz_" + folder_name;

    // Place each run into its own folder based on the CW
    boost::filesystem::create_directory(
        str(boost::format("%s%s") % "/mnt/md0/" % cw_folder));

    boost::filesystem::path base_fn_fp("/mnt/md0/" + cw_folder + "/" + base_fn);
    base_fn_fp.replace_extension(boost::filesystem::path(
        str(boost::format("%s%02d%s%02d%s%02d%s%02d%s") % "tx_" % tx_chan_num % "_rx_"
            % rx_chan_num % "_run_" % run_num % "_cw_" % tx_freq
            % base_fn_fp.extension().string())));

    return base_fn_fp.string();
}


std::string ReceiveFunctions::generateOutFilenameMultithread(const std::string& base_fn,
    const size_t& rx_chan_num,
    const int& tx_chan_num,
    const int& run_num,
    const double& tx_freq,
    const std::string& folder_name,
    const int& threadnum)
{
    // Generates filenames for multithreaded implementation.

    std::string cw_folder =
        "CW_" + std::to_string(tx_freq * 1e-9) + "_GHz_" + folder_name;
    // Todo: This is not dynamic file creation!
    // Place each run into its own folder based on the CW
    // If using a multi-raid system, this code must be changed to accomodate.
    // Change /home/ts-cogrf/workarea/ and /home/ts-cogrf/workarea/ to accomodate your
    // file structure. 1st RAID
    boost::filesystem::create_directory(
        str(boost::format("%s%s") % "/mnt/md0/" % cw_folder));
    // 2nd RAID
    boost::filesystem::create_directory(
        str(boost::format("%s%s") % "/mnt/md1/" % cw_folder));

    boost::filesystem::path base_fn_fp;

    // Direct half of the USRPs to one RAID and half to the other.
    // TODO: THis is hard coded. Need modular.
    if (threadnum > 8) {
        boost::filesystem::path base_fn_fp("/mnt/md0/" + cw_folder + "/" + base_fn);
        base_fn_fp.replace_extension(boost::filesystem::path(
            str(boost::format("%s%02d%s%02d%s%02d%s%02d%s%d%s") % "tx_" % tx_chan_num
                % "_rx_" % rx_chan_num % "_run_" % run_num % "_cw_" % tx_freq % "_thread_"
                % threadnum % base_fn_fp.extension().string())));
        return base_fn_fp.string();
    } else {
        boost::filesystem::path base_fn_fp("/mnt/md1/" + cw_folder + "/" + base_fn);
        base_fn_fp.replace_extension(boost::filesystem::path(
            str(boost::format("%s%02d%s%02d%s%02d%s%02d%s%d%s") % "tx_" % tx_chan_num
                % "_rx_" % rx_chan_num % "_run_" % run_num % "_cw_" % tx_freq % "_thread_"
                % threadnum % base_fn_fp.extension().string())));
        return base_fn_fp.string();
    }
}
