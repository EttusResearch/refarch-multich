#ifndef FILESYSTEM_H
#define FILESYSTEM_H


#include <string>
#include <vector>
#include <thread>

#define NOT_OPEN -1
#define IS_OPEN >= 0


class FileLinux
{
private:
    
    bool background_open_active = false;

protected:
    int fileid = NOT_OPEN;
    std::string file_location;
    std::vector<std::thread> threads;
    bool stop_all_file_threads = false;
    const void *buf;
    size_t buff_size;
    void BACKGROUND_open(int oFlag);
public:
    void openFile(int oFlag);
    virtual ssize_t readFileBlocking(void *buf, size_t nbytes);
    int writeFile(void *buf, size_t nbytes);
    void closeFile();
};


class PipeFile : public FileLinux
{
private:
    int32_t num_of_samples_to_aquire=-1;
    bool pipe_background_process = false;
    void readSampleBlocking();
public:
    int32_t returnedNumberOfSamples(){return num_of_samples_to_aquire;};
    PipeFile(std::string file);
    int readSampleNonBlocking();
    ~PipeFile();
};

#endif
