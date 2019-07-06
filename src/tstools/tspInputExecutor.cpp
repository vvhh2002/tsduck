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
//  Transport stream processor: Execution context of an input plugin
//
//----------------------------------------------------------------------------

#include "tspInputExecutor.h"
#include "tsTime.h"
TSDUCK_SOURCE;

// Minimum number of PID's and PCR/DTS to analyze before getting a valid bitrate.
#define MIN_ANALYZE_PID   1
#define MIN_ANALYZE_PCR  32
#define MIN_ANALYZE_DTS  32


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::tsp::InputExecutor::InputExecutor(Options* options,
                                      const PluginOptions* pl_options,
                                      const ThreadAttributes& attributes,
                                      Mutex& global_mutex) :

    PluginExecutor(options, pl_options, attributes, global_mutex),
    _input(dynamic_cast<InputPlugin*>(PluginThread::plugin())),
    _in_sync_lost(false),
    _instuff_start_remain(options->instuff_start),
    _instuff_stop_remain(options->instuff_stop),
    _instuff_nullpkt_remain(0),
    _instuff_inpkt_remain(0),
    _pcr_analyzer(MIN_ANALYZE_PID, MIN_ANALYZE_PCR),
    _dts_analyzer(),
    _use_dts_analyzer(false)
{
    _dts_analyzer.resetAndUseDTS(MIN_ANALYZE_PID, MIN_ANALYZE_DTS);
}


//----------------------------------------------------------------------------
// Initializes the buffer for all plugin executors.
//----------------------------------------------------------------------------

bool ts::tsp::InputExecutor::initAllBuffers(PacketBuffer* buffer, PacketMetadataBuffer* metadata)
{
    // Pre-declare buffer for input plugin.
    initBuffer(buffer, metadata, 0, buffer->count(), false, false, 0);

    // Pre-load half of the buffer with packets from the input device.
    const size_t pkt_read = receiveAndStuff(0, buffer->count() / 2);

    if (pkt_read == 0) {
        return false; // receive error
    }

    debug(u"initial buffer load: %'d packets, %'d bytes", {pkt_read, pkt_read * PKT_SIZE});

    // Try to evaluate the initial input bitrate.
    const BitRate init_bitrate = getBitrate();
    if (init_bitrate == 0) {
        verbose(u"unknown initial input bitrate");
    }
    else {
        verbose(u"initial input bitrate is %'d b/s", {init_bitrate});
    }

    // Indicate that the loaded packets are now available to the next packet processor.
    PluginExecutor* next = ringNext<PluginExecutor>();
    next->initBuffer(buffer, metadata, 0, pkt_read, pkt_read == 0, pkt_read == 0, init_bitrate);

    // The rest of the buffer belongs to this input processor for reading additional packets.
    initBuffer(buffer, metadata, pkt_read % buffer->count(), buffer->count() - pkt_read, pkt_read == 0, pkt_read == 0, init_bitrate);

    // All other processors have an implicit empty buffer (_pkt_first and _pkt_cnt are zero).
    // Propagate initial input bitrate to all processors
    while ((next = next->ringNext<PluginExecutor>()) != this) {
        next->initBuffer(buffer, metadata, 0, 0, pkt_read == 0, pkt_read == 0, init_bitrate);
    }

    return true;
}


//----------------------------------------------------------------------------
// Encapsulation of the plugin's getBitrate() method or PCR analysis.
//----------------------------------------------------------------------------

ts::BitRate ts::tsp::InputExecutor::getBitrate()
{
    // Get bitrate from --bitrate command line option or from plugin otherwise.
    BitRate bitrate = _options->bitrate > 0 ? _options->bitrate : _input->getBitrate();

    if (bitrate != 0) {
        // Got a bitrate value from command line or plugin.
        if (_options->instuff_inpkt == 0) {
            // No artificial input stuffing, use that bitrate.
            return bitrate;
        }
        else {
            // Need to adjust with artificial input stuffing.
            return BitRate((uint64_t(bitrate) * uint64_t(_options->instuff_nullpkt + _options->instuff_inpkt)) / uint64_t(_options->instuff_inpkt));
        }
    }

    // No valid bitrate from command line or plugin. Evaluate the bitrate ourselves.
    if (!_use_dts_analyzer && _pcr_analyzer.bitrateIsValid()) {
        // Got a bitrate from the PCR's
        return _pcr_analyzer.bitrate188();
    }
    else {
        // Still no bitrate available from PCR, try DTS from video PID's.
        // If DTS are used at least once, continue to use them all the time.
        _use_dts_analyzer = _use_dts_analyzer || _dts_analyzer.bitrateIsValid();
        // Return the bitrate from DTS.
        return _use_dts_analyzer ? _dts_analyzer.bitrate188() : 0;
    }
}


//----------------------------------------------------------------------------
// Receive null packets.
//----------------------------------------------------------------------------

size_t ts::tsp::InputExecutor::receiveNullPackets(size_t index, size_t max_packets)
{
    TSPacket* const pkt = _buffer->base() + index;
    TSPacketMetadata* const data = _metadata->base() + index;

    // Fill the buffer with null packets.
    for (size_t n = 0; n < max_packets; ++n) {
        pkt[n] = NullPacket;
        _pcr_analyzer.feedPacket(pkt[n]);
        _dts_analyzer.feedPacket(pkt[n]);
        data[n].reset();
        data[n].setInputStuffing(true);
    }

    // Count those packets as not coming from the real input plugin.
    addNonPluginPackets(max_packets);
    return max_packets;
}


//----------------------------------------------------------------------------
// Encapsulation of the plugin's receive() method,
// checking the validity of the input.
//----------------------------------------------------------------------------

size_t ts::tsp::InputExecutor::receiveAndValidate(size_t index, size_t max_packets)
{
    // If synchronization lost, report an error
    if (_in_sync_lost) {
        return 0;
    }

    TSPacket* const pkt = _buffer->base() + index;
    TSPacketMetadata* const data = _metadata->base() + index;

    // Reset metadata for new incoming packets.
    for (size_t n = 0; n < max_packets; ++n) {
        data[n].reset();
    }

    // Invoke the plugin receive method
    size_t count = _input->receive(pkt, data, max_packets);

    // Validate sync byte (0x47) at beginning of each packet
    for (size_t n = 0; n < count; ++n) {
        if (pkt[n].hasValidSync()) {
            // Count good packets from plugin
            addPluginPackets(1);

            // Include packet in bitrate analysis.
            _pcr_analyzer.feedPacket(pkt[n]);
            _dts_analyzer.feedPacket(pkt[n]);
        }
        else {
            // Report error
            error(u"synchronization lost after %'d packets, got 0x%X instead of 0x%X", {pluginPackets(), pkt[n].b[0], SYNC_BYTE});
            // In debug mode, partial dump of input
            // (one packet before lost of sync and 3 packets starting at lost of sync).
            if (maxSeverity() >= 1) {
                if (n > 0) {
                    debug(u"content of packet before lost of synchronization:\n%s",
                          {UString::Dump(pkt[n-1].b, PKT_SIZE, UString::HEXA | UString::OFFSET | UString::BPL, 4, 16)});
                }
                const size_t dump_count = std::min<size_t>(3, count - n);
                debug(u"data at lost of synchronization:\n%s",
                      {UString::Dump(pkt[n].b, dump_count * PKT_SIZE, UString::HEXA | UString::OFFSET | UString::BPL, 4, 16)});
            }
            // Ignore subsequent packets
            count = n;
            _in_sync_lost = true;
        }
    }

    return count;
}


//----------------------------------------------------------------------------
// Encapsulation of receiveAndValidate() method,
// taking into account the tsp input stuffing options.
//----------------------------------------------------------------------------

size_t ts::tsp::InputExecutor::receiveAndStuff(size_t index, size_t max_packets)
{
    size_t pkt_done = 0;              // Number of received packets in buffer
    size_t pkt_remain = max_packets;  // Remaining number of packets to read
    size_t pkt_from_input = 0;        // Number of packets actually read from plugin

    // If initial stuffing not yet completed, add initial stuffing.
    while (_instuff_start_remain > 0 && pkt_remain > 0) {
        _buffer->base()[index] = NullPacket;
        _metadata->base()[index].reset();
        _metadata->base()[index].setInputStuffing(true);
        _instuff_start_remain--;
        index++;
        pkt_remain--;
        pkt_done++;
        addNonPluginPackets(1);
    }

    // Now read real packets.
    if (_options->instuff_inpkt == 0) {
        // There is no --add-input-stuffing option, simply call the plugin
        pkt_from_input = receiveAndValidate(index, pkt_remain);
        pkt_done += pkt_from_input;
    }
    else {
        // Otherwise, we have to alternate input packets and null packets.
        while (pkt_remain > 0) {

            // Stuff null packets.
            size_t count = receiveNullPackets(index, std::min(_instuff_nullpkt_remain, pkt_remain));
            _instuff_nullpkt_remain -= count;
            index += count;
            pkt_remain -= count;
            pkt_done += count;

            // Exit on buffer full.
            if (pkt_remain == 0) {
                break;
            }

            // Restart sequence of input packets to read after reading intermediate null packets.
            if (_instuff_nullpkt_remain == 0 && _instuff_inpkt_remain == 0) {
                _instuff_inpkt_remain = _options->instuff_inpkt;
            }

            // Read input packets from the plugin
            const size_t max_input = std::min(pkt_remain, _instuff_inpkt_remain);
            count = receiveAndValidate(index, max_input);
            index += count;
            pkt_remain -= count;
            pkt_done += count;
            pkt_from_input += count;
            _instuff_inpkt_remain -= count;

            // Restart sequence of null packets to stuff after reading chunk of input packets.
            if (_instuff_nullpkt_remain == 0 && _instuff_inpkt_remain == 0) {
                _instuff_nullpkt_remain = _options->instuff_nullpkt;
            }

            // If input plugin returned less than expected, exit now
            if (count < max_input) {
                break;
            }
        }
    }

    // Return number of packets which were added into the packet buffer.
    // In case if end of input, no need to return initial null packets (if any).
    return pkt_from_input == 0 ? 0 : pkt_done;
}


//----------------------------------------------------------------------------
// Input plugin thread
//----------------------------------------------------------------------------

void ts::tsp::InputExecutor::main()
{
    debug(u"input thread started");

    Time current_time(Time::CurrentUTC());
    Time bitrate_due_time(current_time + _options->bitrate_adj);
    PacketCounter bitrate_due_packet = _options->init_bitrate_adj;
    bool plugin_completed = false;
    bool input_end = false;
    bool aborted = false;

    do {
        size_t pkt_first = 0;
        size_t pkt_max = 0;
        BitRate bitrate = 0;
        bool timeout = false;

        // Wait for space in the input buffer.
        // Ignore input_end and bitrate from previous, we are the input processor.
        waitWork(pkt_first, pkt_max, bitrate, input_end, aborted, timeout);

        // If the next thread has given up, give up too since our packets are now useless.
        // Do not even try to add trailing stuffing (--add-stop-stuffing).
        if (aborted) {
            break;
        }

        // In case of abort on timeout, notify previous and next plugin, then exit.
        if (timeout) {
            // Do not progate abort to previous processor since the "previous" one is the output one.
            passPackets(0, _tsp_bitrate, true, false);
            aborted = true;
            break;
        }

        // Do not read more packets than request by --max-input-packets
        if (_options->max_input_pkt > 0 && pkt_max > _options->max_input_pkt) {
            pkt_max = _options->max_input_pkt;
        }

        // Now read at most the specified number of packets (pkt_max).
        size_t pkt_read = 0;

        // Read from the plugin if not already terminated.
        if (!plugin_completed) {
            pkt_read = receiveAndStuff(pkt_first, pkt_max);
            plugin_completed = pkt_read == 0;
        }

        // Read additional trailing stuffing after completion of the input plugin.
        if (plugin_completed && _instuff_stop_remain > 0 && pkt_read < pkt_max) {
            const size_t count = receiveNullPackets(pkt_first + pkt_read, std::min(_instuff_stop_remain, pkt_max - pkt_read));
            pkt_read += count;
            _instuff_stop_remain -= count;
        }

        // Overall input is completed when input plugin and trailing stuffing are completed.
        input_end = plugin_completed && _instuff_stop_remain == 0;

        // Process periodic bitrate adjustment.
        // In initial phase, as long as the bitrate is unknown, retry every init_bitrate_adj packets.
        // Once the bitrate is known, retry every bitrate_adj milliseconds.
        if (_options->bitrate == 0 && ((_tsp_bitrate == 0 && pluginPackets() >= bitrate_due_packet) || (current_time = Time::CurrentUTC()) > bitrate_due_time)) {

            // When bitrate is unknown, retry in a fixed amount of packets.
            if (_tsp_bitrate == 0) {
                do {
                    bitrate_due_packet += _options->init_bitrate_adj;
                } while (bitrate_due_packet <= pluginPackets());
            }

            // Compute time for next bitrate adjustment. Note that we do not
            // use a monotonic time (we use current time and not due time as
            // base for next calculation).
            if (current_time >= bitrate_due_time) {
                bitrate_due_time = current_time + _options->bitrate_adj;
            }

            // Call shared library to get input bitrate
            bitrate = getBitrate();

            if (bitrate > 0) {
                // Keep this bitrate
                _tsp_bitrate = bitrate;
                debug(u"input: got bitrate %'d b/s", {bitrate});
            }
        }

        // Pass received packets to next processor
        passPackets(pkt_read, _tsp_bitrate, input_end, false);

    } while (!input_end);

    // Close the input processor
    _input->stop();

    debug(u"input thread %s after %'d packets", {aborted ? u"aborted" : u"terminated", totalPacketsInThread()});
}
