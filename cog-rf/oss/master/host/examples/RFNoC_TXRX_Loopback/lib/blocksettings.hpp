#ifndef BLOCKSETTINGS_H
#define BLOCKSETTINGS_H

class BlockSettings{

    public:
        static int setRadioRates(GraphSettings& graphSettings, DeviceSettings& deviceSettings);
        static void tuneRX(GraphSettings& graphSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd);
        static void tuneTX(GraphSettings& graphSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd);
        static void setRXGain(GraphSettings& graphSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd);
        static void setTXGain(GraphSettings& graphSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd);
        static void setRXBw(GraphSettings& graphSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd);
        static void setTXBw(GraphSettings& graphSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd);
        static void setRXAnt(GraphSettings& graphSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd);
        static void setTXAnt(GraphSettings& graphSettings, DeviceSettings& deviceSettings, ProgramMetaData& pmd);


};


#endif