#ifndef RECEIVEFUNCTIONS_H
#define RECEIVEFUNCTIONS_H

class ReceiveFunctions{

    public:
        static std::string generateOutFilename(const std::string& base_fn, size_t n_names, size_t rx_chan_num, int tx_chan_num, int repeat_count,
         SignalSettings& signalSettings, DeviceSettings& deviceSettings);
        static std::string generateOutFilenameMultithread(const std::string& base_fn, size_t n_names, size_t rx_chan_num, int tx_chan_num, int repeat_count,
         SignalSettings& signalSettings, DeviceSettings& deviceSettings, int threadnum);
        
        

};

#endif