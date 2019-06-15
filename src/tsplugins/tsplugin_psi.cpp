//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Display PSI/SI information from a transport stream
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsPSILogger.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PSIPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(PSIPlugin);
    public:
        // Implementation of plugin API
        PSIPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        TablesDisplayArgs _display_options;
        PSILoggerArgs     _logger_options;
        TablesDisplay     _display;
        PSILoggerPtr      _logger;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(psi, ts::PSIPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PSIPlugin::PSIPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract PSI Information", u"[options]"),
    _display_options(duck),
    _logger_options(),
    _display(_display_options),
    _logger()
{
    duck.defineOptionsForPDS(*this);
    duck.defineOptionsForStandards(*this);
    duck.defineOptionsForDVBCharset(*this);
    _logger_options.defineOptions(*this);
    _display_options.defineOptions(*this);
}


//----------------------------------------------------------------------------
// Start / stop methods
//----------------------------------------------------------------------------

bool ts::PSIPlugin::getOptions()
{
    // Decode command line options.
    return duck.loadOptions(*this) && _logger_options.load(*this) && _display_options.load(*this);
}


bool ts::PSIPlugin::start()
{
    // Create the logger.
    _logger = new PSILogger(_logger_options, _display);
    return !_logger->hasErrors();
}

bool ts::PSIPlugin::stop()
{
    // Cleanup the logger.
    _logger.clear();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PSIPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _logger->feedPacket(pkt);
    return _logger->completed() ? TSP_END : TSP_OK;
}
