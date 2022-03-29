#ifndef FILESYSTEM_H
#define FILESYSTEM_H


#include <string>
#include <thread>
#include <vector>

#define NOT_OPEN -1

class FileLinux
{
private:
    bool background_open_active = false;

protected:
    int file_id = NOT_OPEN;
    std::string file_location;
    std::vector<std::thread> threads;
    bool stop_all_file_threads = false;
    void backgroundOpen(int oFlag);

public:
    FileLinux(const std::string& file);
    bool isFileOpen()
    {
        return file_id >= 0;
    }
    void openFile(int oFlag);
    ssize_t virtual readFileBlocking(void* buf, size_t nbytes);
    int writeFile(void* buf, size_t nbytes);
    void closeFile();
};


class PipeFile : public FileLinux
{
private:
    bool pipe_background_process = false;
    void readSamplesBlocking(uint16_t num_of_samples);
    std::vector<int32_t> recv_buff;
    ssize_t amount_of_data_returned = 0;
    int32_t* buf;

public:
    std::vector<int32_t> returnedValues()
    {
        return recv_buff;
    }
    bool didBackgroundReturn()
    {
        return amount_of_data_returned > 0;
    }
    void openPipeFile(int oFlag);
    int setfileSize(int file_size);
    PipeFile(const std::string& file);
    int readSamplesNonBlocking(uint16_t num_of_samples);
    ~PipeFile();
};

#endif
