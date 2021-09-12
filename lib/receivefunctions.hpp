#ifndef RECEIVEFUNCTIONS_H
#define RECEIVEFUNCTIONS_H
#include "structures.hpp"

class ReceiveFunctions{

    public:
        static std::string generateOutFilename(const std::string& base_fn, size_t n_names, size_t rx_chan_num, int tx_chan_num, int repeat_count,
         DeviceSettings& deviceSettings);
        static std::string generateOutFilenameMultithread(const std::string& base_fn, size_t n_names, size_t rx_chan_num, int tx_chan_num, int repeat_count,
         DeviceSettings& deviceSettings, int threadnum);
        
        

};

#endif