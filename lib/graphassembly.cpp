
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/rfnoc/radio_control.hpp>

#include <iostream>
#include <thread>
#include <boost/format.hpp>
#include "structures.hpp"
#include "graphassembly.hpp"



void GraphAssembly::buildGraph(GraphSettings& graphSettings, const DeviceSettings& deviceSettings){

    //Create RFNoC Graph

     /************************************************************************
     * Create device and block controls
     ***********************************************************************/

    // If multiple USRPs are used, they are linked into a single RFNoc graph here. 
    std::cout << std::endl;
    std::cout << "Creating the RFNoC graph with args: " << deviceSettings.args << "..." << std::endl;
    graphSettings.graph = uhd::rfnoc::rfnoc_graph::make(deviceSettings.args);
    
    
    

}

void GraphAssembly::buildRadios(GraphSettings& graphSettings){

    /************************************************************************
     * Seek radio blocks on each USRP and assemble a vector of radio 
     * controllers.  
     ***********************************************************************/
    std::cout << "Building Radios..." << std::endl;
    //Make radio controllers for multiple channels/devices
    graphSettings.radio_block_list = graphSettings.graph->find_blocks("Radio");
    
    // Iterate over each radio block found on each device
    for (auto& elem: graphSettings.radio_block_list){
        // Create a vector of radio control objects for controlling the radio blocks
        graphSettings.radio_ctrls.push_back(graphSettings.graph->get_block<uhd::rfnoc::radio_control>(elem));
        

        std::cout << "Using radio " << elem << std::endl;


    }
    // Sort the vectors
    sort(graphSettings.radio_ctrls.begin(),graphSettings.radio_ctrls.end());
    sort(graphSettings.radio_block_list.begin(),graphSettings.radio_block_list.end());
    
    
}

void GraphAssembly::buildDDCDUC(GraphSettings& graphSettings){

    /*************************************************************************
    * Seek DDCs & DUCs on each USRP and assemble a vector of DDC & DUC controllers.
    ************************************************************************/
    std::cout << "Building DDC/DUCs.." << std::endl;

    std::vector<std::string> ddc_sources;
    std::vector<std::string> duc_destinations;

    // Enumerate blocks in the chain
    auto edges = graphSettings.graph->enumerate_static_connections();
   
    //vector of string identifiers for DDC destinations
    for (auto& rctrl : graphSettings.radio_ctrls){
        ddc_sources.push_back(rctrl->get_block_id());
        
        duc_destinations.push_back(rctrl->get_block_id());
    }

    sort(ddc_sources.begin(),ddc_sources.end());


  
    size_t ddc_source_port = 0;
    size_t duc_dst_port = 0;
    auto chain               = std::vector<uhd::rfnoc::graph_edge_t>();
    
    
    graphSettings.ddc_chan = 0;
    graphSettings.duc_chan = 0;

    //Find DDCs connected to the radios 
    
    // There is a single channel (src_port) included on the stock DDCs & DUCs on the N3xx XG image, 0.
    
    // Look at each connection in the RFNoc graph
    for (auto& edge : edges) {
        // Loop over the radio blocks and look for connected DDCs
        for (auto& src_block : ddc_sources){
            if (edge.src_blockid == src_block && edge.src_port == ddc_source_port) {
                auto blockid = uhd::rfnoc::block_id_t(edge.dst_blockid);
                if (blockid.match("DDC")) {
                    // When a DUC is found, assemble a vector of DDC controllers.
                    graphSettings.ddc_ctrls.push_back(graphSettings.graph->get_block<uhd::rfnoc::ddc_block_control>(blockid));
                }
            }
            
        }

        for (auto& dst_block : duc_destinations){
            if (edge.dst_blockid == dst_block && edge.dst_port == duc_dst_port) {
                auto blockid = uhd::rfnoc::block_id_t(edge.src_blockid);
                if (blockid.match("DUC")) {
                    // When a DUC is found, assemble a vector of DUC controllers.
                    graphSettings.duc_ctrls.push_back(graphSettings.graph->get_block<uhd::rfnoc::duc_block_control>(blockid));
    
                }
            }
        }
        chain.push_back(edge);
    }
    //Sort Vector
    sort(graphSettings.ddc_ctrls.begin(),graphSettings.ddc_ctrls.end());
    sort(graphSettings.duc_ctrls.begin(),graphSettings.duc_ctrls.end());
    // For Display Purposes
    for (auto& dctrl : graphSettings.ddc_ctrls){
            std::cout << "Using DDC " << dctrl->get_block_id() << ", Channel " << graphSettings.ddc_chan << std::endl;
    }
    // For Display Purposes
    for (auto& dctrl : graphSettings.duc_ctrls){
            std::cout << "Using DUC " << dctrl->get_block_id() << ", Channel " << graphSettings.duc_chan << std::endl;
    }


}

void GraphAssembly::buildReplay(GraphSettings& graphSettings){

    /****************************************************************************
     * Seek Replay blocks on each USRP and assemble a vector of Replay Block Controllers
     ***************************************************************************/

    // Check what replay blocks exist on the device(s)
    graphSettings.replay_block_list = graphSettings.graph->find_blocks("Replay");
    
    
    for (auto& replay_id : graphSettings.replay_block_list){
        // The double entry is intended. There are two channels on each replay block. 
        graphSettings.replay_ctrls.push_back(graphSettings.graph->get_block<uhd::rfnoc::replay_block_control>(replay_id));
        graphSettings.replay_ctrls.push_back(graphSettings.graph->get_block<uhd::rfnoc::replay_block_control>(replay_id));
        
    }
    sort(graphSettings.replay_ctrls.begin(), graphSettings.replay_ctrls.end());
    
    // Each replay block has two channels, 0 and 1.
    // Vector of replay channels. 
    for (int i_chan = 0 ; i_chan < graphSettings.replay_ctrls.size()*2; i_chan++){
        if (i_chan%2 == 0){
            graphSettings.replay_chan_vector.push_back(0);
        }
        else{
            graphSettings.replay_chan_vector.push_back(1);
        }
    }

    //For Display Purposes
    int count_replay_chan = 0;
    for (auto& rctrl : graphSettings.replay_ctrls){
        std::cout << "Using Replay " << rctrl->get_block_id() << ", Channel " << graphSettings.replay_chan_vector[count_replay_chan] << std::endl;
        count_replay_chan++;
        
    }

}

void GraphAssembly::buildStreams(GraphSettings& graphSettings, DeviceSettings& deviceSettings, SignalSettings& signalSettings){
    //Build streams for single threaded implementation. 

    // Constants related to the Replay block
    const size_t replay_word_size = 8; // Size of words used by replay block
    const size_t sample_size      = 4; // Complex signed 16-bit is 32 bits per sample
    const size_t samples_per_word = 2; // Number of sc16 samples per word

    uhd::device_addr_t streamer_args(deviceSettings.streamargs);
    
    

    // create a receive streamer
    uhd::stream_args_t stream_args(signalSettings.format, signalSettings.otw); 
    stream_args.args = streamer_args;
    std::cout << "Using streamer args: " << stream_args.args.to_string() << std::endl;
    
    if (signalSettings.singleTXRX_loopback == true){
        graphSettings.rx_stream = graphSettings.graph->create_rx_streamer(1, stream_args);
    }
    else{
        graphSettings.rx_stream = graphSettings.graph->create_rx_streamer(graphSettings.radio_ctrls.size(), stream_args);
    }


    /************************************************************************
     * Set up streamer to Replay blocks
     ***********************************************************************/

    
    for (int i_s2r = 0; i_s2r < graphSettings.replay_ctrls.size(); i_s2r = i_s2r + 2){
        streamer_args["block_id"]   = graphSettings.replay_ctrls[i_s2r]->get_block_id().to_string();
        streamer_args["block_port"] = std::to_string(0);
        stream_args.args            = streamer_args;
        stream_args.channels        = {0};
        
        graphSettings.tx_stream = graphSettings.graph->create_tx_streamer(stream_args.channels.size(), stream_args);
        size_t tx_spp = graphSettings.tx_stream->get_max_num_samps();
        //Make sure that stream SPP is a multiple of the Replay Block Word Size
        if (tx_spp % samples_per_word != 0) {
            // Round SPP down to a multiple of the word size
            tx_spp = (tx_spp / samples_per_word) * samples_per_word;
            streamer_args["spp"] = std::to_string(tx_spp);
            stream_args.args     = streamer_args;
            graphSettings.tx_stream = graphSettings.graph->create_tx_streamer(stream_args.channels.size(), stream_args);

        }
        // Vector of tx streamers, duplicate for vector length padding. 
        graphSettings.tx_stream_vector.push_back(graphSettings.tx_stream);
        graphSettings.tx_stream_vector.push_back(graphSettings.tx_stream);
}
}

void GraphAssembly::connectGraph(GraphSettings& graphSettings, SignalSettings& signalSettings){

    //This is the connect function for single threaded implementation. 

    if (signalSettings.singleTXRX_loopback == false){

        UHD_LOG_INFO("CogRF", "Connecting graph...");

    //Connect Graph
    for (int i = 0; i < graphSettings.radio_ctrls.size(); i++){
        //connect radios to ddc
        graphSettings.graph->connect(graphSettings.radio_block_list[i], 0, graphSettings.ddc_ctrls[i]->get_block_id(),0);
        std::cout << "Connected "  << graphSettings.radio_block_list[i] << " to " << graphSettings.ddc_ctrls[i]->get_block_id() << std::endl;
        graphSettings.graph->connect(graphSettings.ddc_ctrls[i]->get_block_id(), 0, graphSettings.rx_stream,i);
        std::cout << "Connected " << graphSettings.ddc_ctrls[i]->get_block_id() << " to " << graphSettings.rx_stream << " Port " << i << std::endl;
    }

    int pos2 = 0;
    if (graphSettings.duc_ctrls.size() > 0){
       
        
        for (auto& rctrl : graphSettings.radio_ctrls){
            graphSettings.graph->connect(
                graphSettings.duc_ctrls[pos2]->get_block_id(), graphSettings.duc_chan, rctrl->get_block_id(), 0);
            std::cout << "Connected " << graphSettings.duc_ctrls[pos2]->get_block_id() << " port " 
            << graphSettings.duc_chan << " to radio " << rctrl->get_block_id() << " port " << 0 << std::endl;
            
           
            pos2++;
            
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        for (int i_r2d = 0; i_r2d < graphSettings.duc_ctrls.size(); i_r2d++){
            // Catch timeout exception issue. 
            while(true){
                try{
                    graphSettings.graph->connect(
                    graphSettings.replay_ctrls[i_r2d]->get_block_id(), graphSettings.replay_chan_vector[i_r2d], graphSettings.duc_ctrls[i_r2d]->get_block_id(), graphSettings.duc_chan);
                    break;
                } catch (...){
                    //std::this_thread::sleep_for(std::chrono::milliseconds(500));

                }
            }
            std::cout << "Connected " <<  graphSettings.replay_ctrls[i_r2d]->get_block_id() << " port " 
            << graphSettings.replay_chan_vector[i_r2d] << " to DUC " << graphSettings.duc_ctrls[i_r2d]->get_block_id() << " port " << graphSettings.duc_chan << std::endl;
        
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    } else {
        int pos2 = 0;
        for (auto& replctrl : graphSettings.replay_ctrls){
            for (auto& rctrl : graphSettings.radio_ctrls){
                while(true){
                    try{
                    graphSettings.graph->connect(replctrl->get_block_id(),
                        graphSettings.replay_chan_vector[pos2],
                        rctrl->get_block_id(),
                        0);
                        break;
                    }catch(...){
                        //std::this_thread::sleep_for(std::chrono::milliseconds(500));

                    }
                }
                
                    pos2++;
                
            }
        }
    }
    for (int i_s2r = 0; i_s2r < graphSettings.replay_ctrls.size(); i_s2r = i_s2r + 2){
        
        graphSettings.graph->connect(graphSettings.tx_stream_vector[i_s2r], 0, graphSettings.replay_ctrls[i_s2r]->get_block_id(), 0);
        std::cout << "Streamer: " << graphSettings.tx_stream_vector[i_s2r] << " connected to " << graphSettings.replay_ctrls[i_s2r]->get_block_id() << std::endl;
        

    }
    }
    else{
        //Single Loopback Graph
        //connect radios to ddc
        std::cout << "SINGLE TXRX" << std::endl;
        UHD_LOG_INFO("CogRF", "Connecting graph...");
        graphSettings.graph->connect(graphSettings.radio_block_list[signalSettings.singleRX], 0, graphSettings.ddc_ctrls[signalSettings.singleRX]->get_block_id(),0);
        std::cout << "Connected "  << graphSettings.radio_block_list[signalSettings.singleRX] << " to " << graphSettings.ddc_ctrls[signalSettings.singleRX]->get_block_id() << std::endl;
        graphSettings.graph->connect(graphSettings.ddc_ctrls[signalSettings.singleRX]->get_block_id(), 0, graphSettings.rx_stream,0);
        std::cout << "Connected " << graphSettings.ddc_ctrls[signalSettings.singleRX]->get_block_id() << " to " << graphSettings.rx_stream << " Port " << 0 << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        graphSettings.graph->connect(graphSettings.duc_ctrls[signalSettings.singleTX]->get_block_id(), graphSettings.duc_chan, graphSettings.radio_ctrls[signalSettings.singleTX]->get_block_id(), 0);
        std::cout << "Connected " << graphSettings.duc_ctrls[signalSettings.singleTX]->get_block_id() << " port " 
        << graphSettings.duc_chan << " to radio " << graphSettings.radio_ctrls[signalSettings.singleTX]->get_block_id() << " port " << 0 << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        graphSettings.graph->connect(graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id(), graphSettings.replay_chan_vector[signalSettings.singleTX], graphSettings.duc_ctrls[signalSettings.singleTX]->get_block_id(), graphSettings.duc_chan);
        std::cout << "Connected " <<  graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << " port " << graphSettings.replay_chan_vector[signalSettings.singleTX] << " to DUC " << graphSettings.duc_ctrls[signalSettings.singleTX]->get_block_id() << " port " << graphSettings.duc_chan << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        graphSettings.graph->connect(graphSettings.tx_stream_vector[signalSettings.singleTX], 0, graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id(), 0);
        std::cout << "Streamer: " << graphSettings.tx_stream_vector[signalSettings.singleTX] << " connected to " << graphSettings.replay_ctrls[signalSettings.singleTX]->get_block_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    }

}

void GraphAssembly::commitGraph(GraphSettings& graphSettings){
    UHD_LOG_INFO("CogRF", "Committing graph...");
    graphSettings.graph->commit();
    UHD_LOG_INFO("CogRF", "Commit complete.");
}




void GraphAssembly::connectGraphMultithread(GraphSettings& graphSettings){
    //This is the function that connects the graph for the multithreaded implementation
    //The difference is that each channel gets its own RX streamer. 

    UHD_LOG_INFO("CogRF", "Connecting graph...");

    //Connect Graph
    for (int i = 0; i < graphSettings.radio_ctrls.size(); i++){
        //connect radios to ddc
        graphSettings.graph->connect(graphSettings.radio_block_list[i], 0, graphSettings.ddc_ctrls[i]->get_block_id(),0);
        std::cout << "Connected "  << graphSettings.radio_block_list[i] << " to " << graphSettings.ddc_ctrls[i]->get_block_id() << std::endl;
        
    }

    for (int j = 0; j < graphSettings.ddc_ctrls.size(); j++){
        //Connect DDC to streamers
        //Reusing replay chan vector, need a vector of zeros and ones 
        graphSettings.graph->connect(graphSettings.ddc_ctrls[j]->get_block_id(), 0, graphSettings.rx_stream_vector[j], graphSettings.replay_chan_vector[j]);
        std::cout << "Connected " << graphSettings.ddc_ctrls[j]->get_block_id() << " to " << graphSettings.rx_stream_vector[j] << " Port " << graphSettings.replay_chan_vector[j] << std::endl;

    }

    int pos2 = 0;
    if (graphSettings.duc_ctrls.size() > 0){
    
        
        for (auto& rctrl : graphSettings.radio_ctrls){
            graphSettings.graph->connect(
                graphSettings.duc_ctrls[pos2]->get_block_id(), graphSettings.duc_chan, rctrl->get_block_id(), 0);
            std::cout << "Connected " << graphSettings.duc_ctrls[pos2]->get_block_id() << " port " 
            << graphSettings.duc_chan << " to radio " << rctrl->get_block_id() << " port " << 0 << std::endl;
            
        
            pos2++;
            
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        for (int i_r2d = 0; i_r2d < graphSettings.duc_ctrls.size(); i_r2d++){
           
            graphSettings.graph->connect(
            graphSettings.replay_ctrls[i_r2d]->get_block_id(), graphSettings.replay_chan_vector[i_r2d], graphSettings.duc_ctrls[i_r2d]->get_block_id(), graphSettings.duc_chan);
            
                
            std::cout << "Connected " <<  graphSettings.replay_ctrls[i_r2d]->get_block_id() << " port " 
            << graphSettings.replay_chan_vector[i_r2d] << " to DUC " << graphSettings.duc_ctrls[i_r2d]->get_block_id() << " port " << graphSettings.duc_chan << std::endl;
        
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    } else {
        int pos2 = 0;
        for (auto& replctrl : graphSettings.replay_ctrls){
            for (auto& rctrl : graphSettings.radio_ctrls){
                
                    
                graphSettings.graph->connect(replctrl->get_block_id(),
                graphSettings.replay_chan_vector[pos2],rctrl->get_block_id(), 0);
            
                pos2++;
                
            }
        }
    }
    for (int i_s2r = 0; i_s2r < graphSettings.replay_ctrls.size(); i_s2r = i_s2r + 2){
        
        graphSettings.graph->connect(graphSettings.tx_stream_vector[i_s2r], 0, graphSettings.replay_ctrls[i_s2r]->get_block_id(), 0);
        std::cout << "Streamer: " << graphSettings.tx_stream_vector[i_s2r] << " connected to " << graphSettings.replay_ctrls[i_s2r]->get_block_id() << std::endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
 
    }

   

}

void GraphAssembly::buildStreamsMultithread(GraphSettings& graphSettings, DeviceSettings& deviceSettings, SignalSettings& signalSettings){
    //Build Streams for multithreaded implementation
    //Each Channel gets its own RX streamer. 

    // Constants related to the Replay block
    const size_t replay_word_size = 8; // Size of words used by replay block
    const size_t sample_size      = 4; // Complex signed 16-bit is 32 bits per sample
    const size_t samples_per_word = 2; // Number of sc16 samples per word

    uhd::device_addr_t streamer_args(deviceSettings.streamargs);
    
    

    // create a receive streamer
    // std::cout << "Samples per packet: " << spp << std::endl;
    uhd::stream_args_t stream_args(signalSettings.format, signalSettings.otw); // We should read the wire format from the blocks
    stream_args.args = streamer_args;
    std::cout << "Using streamer args: " << stream_args.args.to_string() << std::endl;
    
    
    //One stream per channel
    for (int rx_count = 0; rx_count < graphSettings.radio_ctrls.size()/2; rx_count++){
        graphSettings.rx_stream = graphSettings.graph->create_rx_streamer(2, stream_args);
        graphSettings.rx_stream_vector.push_back(graphSettings.rx_stream);
        graphSettings.rx_stream_vector.push_back(graphSettings.rx_stream);
        }

    /************************************************************************
     * Set up streamer to Replay blocks
     ***********************************************************************/

    
    for (int i_s2r = 0; i_s2r < graphSettings.replay_ctrls.size(); i_s2r = i_s2r + 2){
        streamer_args["block_id"]   = graphSettings.replay_ctrls[i_s2r]->get_block_id().to_string();
        streamer_args["block_port"] = std::to_string(0);
        stream_args.args            = streamer_args;
        stream_args.channels        = {0};
        
        graphSettings.tx_stream = graphSettings.graph->create_tx_streamer(stream_args.channels.size(), stream_args);
        size_t tx_spp = graphSettings.tx_stream->get_max_num_samps();
        //Make sure that stream SPP is a multiple of the Replay Block Word Size
        if (tx_spp % samples_per_word != 0) {
            // Round SPP down to a multiple of the word size
            tx_spp = (tx_spp / samples_per_word) * samples_per_word;
            streamer_args["spp"] = std::to_string(tx_spp);
            stream_args.args     = streamer_args;
            graphSettings.tx_stream = graphSettings.graph->create_tx_streamer(stream_args.channels.size(), stream_args);

        }
        // Vector of tx streamers, duplicate for vector length padding. 
        graphSettings.tx_stream_vector.push_back(graphSettings.tx_stream);
        graphSettings.tx_stream_vector.push_back(graphSettings.tx_stream);
}
}