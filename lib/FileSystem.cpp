
#include <boost/filesystem.hpp>
#include <thread>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include "FileSystem.hpp"

namespace RA_filesystem = boost::filesystem; //Currently Only tested on Linux

/**
 * @brief Background process for opening file tries every 100ms
 * 
 * @param oFlag Flag that is | with O_NONBLOCK
 */
void FileLinux::BACKGROUND_open(int oFlag){
    do {
        fileid = open(file_location.c_str(), oFlag | O_NONBLOCK);
        //Delay to give some time for reader or writer to catchup.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }while( fileid == NOT_OPEN && !stop_all_file_threads );
    background_open_active = false;
}
/**
 * @brief Spawns background process to open file. This could take a few ms.
 * 
 * @param oFlag IO flag (O_WRONLY, O_RDONLY)
 */
void FileLinux::openFile(int oFlag){
    if (!background_open_active){
        background_open_active = true; 
        std::thread t(
        [this](int oFlag){BACKGROUND_open(oFlag);}
        ,oFlag);
        threads.push_back(std::move(t));
    }
}
/**
 * @brief Automatically opens and tries to read file.
 * 
 * 
 * @param buf Buffer to populate
 * @param nbytes Number of values 
 * @return int - Returns the number read, -1 for errors or 0 for EOF.
 */
ssize_t FileLinux::readFileBlocking(void *buf, size_t nbytes){
    if (fileid IS_OPEN ) return read(fileid, buf, nbytes);
    else if(!background_open_active){
        std::cout <<"start background read"<<std::endl << std::flush;
        openFile(O_RDONLY);
        //delay to open the File and try again
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return readFileBlocking(buf, nbytes);
    }
    else return NOT_OPEN;
}
/**
 * @brief Automatically opens and tries to write to file.
 * 
 * @param buf Buffer to populate
 * @param nbytes Number of values
 * @return int Returns the number written, -1 for errors.
 */
int FileLinux::writeFile(void *buf, size_t nbytes){
    if (fileid IS_OPEN ) {
        return write(fileid, buf, nbytes);
    }
    else if(!background_open_active){
        openFile(O_WRONLY);
        //delay to open the File and try again
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return writeFile(buf, nbytes);
    }
    else return NOT_OPEN;
}

/**
 * @brief Checks to see if file is open then closes.
 * 
 */
void FileLinux::closeFile(){ 
    if (fileid IS_OPEN) 
    {
        close(fileid);
        fileid = NOT_OPEN;
    } 
}


/**
 * @brief Construct a new Pipe File object
 * 
 * @param file Absolute path to make Pipe
 */
PipeFile::PipeFile(std::string file){
    file_location=file;
    //todo: There is no check to make sure file is created.
    if (!RA_filesystem::exists(file)){
    mkfifo(file_location.c_str(), 0666);
    }
}

int PipeFile::readSampleNonBlocking() {
    if (!pipe_background_process){
        pipe_background_process = true;
        num_of_samples_to_aquire = -1;
        std::thread t([this](){readSampleBlocking();});
        threads.push_back(std::move(t));
        return 0;
    }
    else return -1;
}

void PipeFile::readSampleBlocking(){
    u_int16_t readData = -1;
    do{
    readFileBlocking(&readData, sizeof(readData));
    }while(readData == -1);
    num_of_samples_to_aquire = readData;
    pipe_background_process = false;
}

/**
* @brief Destroy the Pipe File object
*/
PipeFile::~PipeFile(){
    stop_all_file_threads = true;
    closeFile();
    unlink(file_location.c_str());
    for (auto& thread: threads){
    thread.join();
    }
}
