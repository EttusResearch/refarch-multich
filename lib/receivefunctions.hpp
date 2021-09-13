#ifndef RECEIVEFUNCTIONS_H
#define RECEIVEFUNCTIONS_H
#include "structures.hpp"

class ReceiveFunctions{

    public:
        static std::string generateOutFilename(const std::string& base_fn, const size_t rx_chan_num, const int tx_chan_num, const int run_num, 
            const double tx_freq, const std::string folder_name);
        static std::string generateOutFilenameMultithread(const std::string& base_fn, size_t rx_chan_num, int tx_chan_num, int run_num, 
            const double tx_freq, const std::string folder_name, int threadnum);
        
        

};

#endif