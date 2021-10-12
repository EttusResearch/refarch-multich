//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef GRAPHASSEMBLY_H
#define GRAPHASSEMBLY_H
#include "structures.hpp"


class GraphAssembly
{
public:
    // methods
    static void buildGraph(GraphSettings& graphSettings, const std::string& args);
    static void buildRadios(GraphSettings& graphSettings);
    static void buildDDCDUC(GraphSettings& graphSettings);
    static void buildReplay(GraphSettings& graphSettings);
    static void buildStreams(GraphSettings& graphSettings,
        const std::string& streamargs,
        const std::string& format,
        const std::string& otw);
    static void connectGraph(GraphSettings& graphSettings);
    static void commitGraph(GraphSettings& graphSettings);
    static void connectGraphMultithread(GraphSettings& graphSettings);
    static void buildStreamsMultithread(GraphSettings& graphSettings,
        const std::string& streamargs,
        const std::string& format,
        const std::string& otw);
};

#endif