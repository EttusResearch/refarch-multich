
#include "FileSystem.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <boost/filesystem.hpp>
#include <iostream>
#include <thread>
//#include <uhd/transport/buffer_pool.hpp> Might want to reuse buffer creation for higher
//throughput


namespace RA_filesystem = boost::filesystem; // Currently Only tested on Linux

/**
 * @brief Construct a new File Linux:: File Linux object
 *
 * @param file Sets file_location to file
 */
FileLinux::FileLinux(const std::string& file)
{
    file_location = file;
}

/**
 * @brief Background process for opening file tries every 100ms
 *
 * @param oFlag Flag that is | with O_NONBLOCK
 */
void FileLinux::backgroundOpen(int oFlag)
{
    do {
        file_id = open(file_location.c_str(), oFlag | O_NONBLOCK);
        // Delay to give some time for reader or writer to catchup.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (!isFileOpen() && !stop_all_file_threads);
    background_open_active = false;
}
/**
 * @brief Spawns background process to open file. This could take a few ms.
 *
 * @param oFlag IO flag (O_WRONLY, O_RDONLY)
 */
void FileLinux::openFile(int oFlag)
{
    stop_all_file_threads = false;
    if (!background_open_active) {
        background_open_active = true;
        std::thread t([this](int oFlag) { backgroundOpen(oFlag); }, oFlag);
        threads.push_back(std::move(t));
    }
}
/**
 * @brief Automatically opens and tries to read file.
 *
 *
 * @param buf Buffer to populate
 * @param n_bytes Number of values
 * @return int - Returns the number read, -1 for errors or 0 for EOF.
 */
ssize_t FileLinux::readFileBlocking(void* buf, size_t n_bytes)
{
    if (isFileOpen())
        return read(file_id, buf, n_bytes);
    else if (!background_open_active) {
        openFile(O_RDONLY);
        // delay to open the File and try again
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return readFileBlocking(buf, n_bytes);
    } else
        return isFileOpen();
}
/**
 * @brief Automatically opens and tries to write to file.
 *
 * @param buf Buffer to populate
 * @param n_bytes Number of values
 * @return int Returns the number written, -1 for errors.
 */
int FileLinux::writeFile(void* buf, size_t n_bytes)
{
    if (isFileOpen()) {
        return write(file_id, buf, n_bytes);
    } else if (!background_open_active) {
        openFile(O_WRONLY);
        // delay to open the File and try again
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return writeFile(buf, n_bytes);
    } else
        return isFileOpen();
}

/**
 * @brief Checks to see if file is open then closes.
 *
 */
void FileLinux::closeFile()
{
    stop_all_file_threads = true;
    if (isFileOpen()) {
        close(file_id);
        file_id = NOT_OPEN;
    }
}


/**
 * @brief Construct a new Pipe File object
 *
 * @param file Absolute path to make Pipe
 */
PipeFile::PipeFile(const std::string& file) : FileLinux(file)
{
    // todo: There is no check to make sure file is created.
    if (!RA_filesystem::exists(file)) {
        mkfifo(file_location.c_str(), 0666);
    }
}
/**
 * @brief Starts a background process to read num_of_samples from file
 *
 * @param num_of_samples number of samples to read of size int32
 * @return int
 */
int PipeFile::readSamplesNonBlocking(uint16_t num_of_samples)
{
    if (!pipe_background_process) {
        pipe_background_process = true;
        amount_of_data_returned = 0;
        std::thread t(
            [this](uint16_t num_of_samples) { readSamplesBlocking(num_of_samples); },
            num_of_samples);
        threads.push_back(std::move(t));
        return 0;
    } else
        return -1;
}
/**
 * @brief reads the samples blocking unless stop_all_file_threads or the
 *          data is returned
 *
 * @param num_of_samples
 */
void PipeFile::readSamplesBlocking(uint16_t num_of_samples)
{
    recv_buff.clear();
    recv_buff.resize(num_of_samples);
    buf = &recv_buff.front();
    do {
        amount_of_data_returned = readFileBlocking(buf, sizeof(int32_t) * num_of_samples);
    } while (amount_of_data_returned <= 0 && !stop_all_file_threads);
    pipe_background_process = false;
}

/**
 * @brief Sets the pipe capacity and returns the resulting value.
 *          The capacity can be increased further by changing the limit in
 * /proc/sys/fs/pipe-max-size
 *
 * @param file_size
 * @return int Returns the current size
 */
int PipeFile::setfileSize(int file_size)
{
    fcntl(file_id, F_SETPIPE_SZ, file_size);
    return fcntl(file_id, F_GETPIPE_SZ);
}

/**
 * @brief Destroy the Pipe File:: Pipe File object
 *
 */
PipeFile::~PipeFile()
{
    stop_all_file_threads = true;
    closeFile();
    unlink(file_location.c_str());
    for (auto& thread : threads) {
        thread.join();
    }
}
