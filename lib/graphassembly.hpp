#ifndef GRAPHASSEMBLY_H
#define GRAPHASSEMBLY_H
#include "structures.hpp"

/**
 * @brief These functions update the graph
 * 
 */
class GraphAssembly
{
    
    public:
        //functions
        /**
         * @brief Create device and block controls
         * 
         * If multiple USRPs are used, they are linked into a single RFNoc graph.
         * 
         * @param graphSettings 
         * @param args Address(es) of USRP(s)
         */
        static void buildGraph(GraphSettings& graphSettings, const std::string& args);


        /**
         * @brief Seek radio blocks on each USRP and assemble a vector of radio
         *        controllers.
         * 
         * updates the graphSettings with
         *      - Aquires all USRP blocks into -> radio_block_list
         *      - Adds all radio_ctrls for the USRPs
         * 
         * and sorts both radio_block_list and _radio_ctrls.
         * 
         * @param graphSettings Sets the radio_block_list
         */
        static void buildRadios(GraphSettings& graphSettings);

        /**
         * @brief 
         * 
         * @param graphSettings 
         */
        static void buildDDCDUC(GraphSettings& graphSettings);
        static void buildReplay(GraphSettings& graphSettings); 
        static void buildStreams(GraphSettings& graphSettings, const std::string& streamargs, const std::string& format, const std::string& otw); 
        static void connectGraph(GraphSettings& graphSettings);
        static void commitGraph(GraphSettings& graphSettings);
        static void connectGraphMultithread(GraphSettings& graphSettings);
        static void buildStreamsMultithread(GraphSettings& graphSettings, const std::string& streamargs, const std::string& format, const std::string& otw);
};

#endif