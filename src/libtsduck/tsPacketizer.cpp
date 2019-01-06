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
//  Packetization of MPEG sections into Transport Stream packets.
//
//----------------------------------------------------------------------------

#include "tsPacketizer.h"
#include "tsNames.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::Packetizer::Packetizer(PID pid, SectionProviderInterface* provider) :
    _provider(provider),
    _pid(pid),
    _continuity(0),
    _section(nullptr),
    _next_byte(0),
    _packet_count(0),
    _section_out_count(0),
    _section_in_count(0)
{
}


//----------------------------------------------------------------------------
// Reset the content of a packetizer. Becomes empty.
// If the last returned packet contained an unfinished
// section, this section will be lost.
//----------------------------------------------------------------------------

void ts::Packetizer::reset()
{
    _section.clear();
    _next_byte = 0;
}


//----------------------------------------------------------------------------
// Build the next MPEG packet for the list of sections.
//----------------------------------------------------------------------------

bool ts::Packetizer::getNextPacket(TSPacket& pkt)
{
    // Count generated packets
    _packet_count++;

    // If there is no current section, get the next one.
    if (_section.isNull() && _provider != nullptr) {
        _provider->provideSection(_section_in_count++, _section);
        _next_byte = 0;
    }

    // If there is still no current section, return a null packet
    if (_section.isNull()) {
        pkt = NullPacket;
        return false;
    }

    // Various values to build the MPEG header.
    uint16_t pusi = 0x0000;         // payload_unit_start_indicator (set: 0x4000)
    uint8_t pointer_field = 0x00;   // pointer_field (used only if pusi is set)
    size_t remain_in_section = _section->size() - _next_byte;
    bool do_stuffing = true;        // do we need to insert stuffing at end of packet?
    SectionPtr next_section;        // next section after current one

    // Check if it is possible that a new section may start in the middle
    // of the packet. We check that after adding the remaining of the
    // current section, there is room for a pointer field (5 = 4-byte TS header
    // + 1-byte pointer field) and at least a short section header.
    // Remember that we are kind enough not to break a section header across packets.

    if (remain_in_section <= PKT_SIZE - 5 - SHORT_SECTION_HEADER_SIZE) {
        // Check if next section requires stuffing before it.
        do_stuffing = _provider == nullptr ? true : _provider->doStuffing();
        if (!do_stuffing) {
            // No stuffing before next section => get next section
            _provider->provideSection(_section_in_count++, next_section);
            if (next_section.isNull()) {
                // If no next section, do stuffing anyway.
                do_stuffing = true;
            }
            else {
                // Now that we know the actual header size of the next section,
                // recheck if it fits in packet
                do_stuffing = remain_in_section > PKT_SIZE - 5 - next_section->headerSize();
            }
        }
    }

    // Do we need to insert a pointer_field?

    if (_next_byte == 0) {
        // We are at the beginning of a section
        pusi = 0x4000;
        pointer_field = 0x00; // section starts immediately after pointer field
    }
    else if (!do_stuffing) {
        // A new section will start in the middle of the packet
        pusi = 0x4000;
        pointer_field = uint8_t(remain_in_section);  // point after current section
    }

    // Build the header

    pkt.b[0] = SYNC_BYTE;
    PutUInt16(pkt.b + 1, (pusi | _pid));
    pkt.b[3] = 0x10 | _continuity; // 0x10 = no adaptation field, has payload

    // Update continuity counter for next packet

    _continuity = (_continuity + 1) & 0x0F;

    // Remaining bytes in the packet.

    uint8_t* data = pkt.b + 4;
    size_t remain_in_packet = PKT_SIZE - 4;

    // Insert the pointer field if required.

    if (pusi) {
        *data++ = pointer_field;
        remain_in_packet--;
    }

    // Fill the packet payload

    while (remain_in_packet > 0) {
        // Copy a part of the current section in the packet
        size_t length = remain_in_section < remain_in_packet ? remain_in_section : remain_in_packet;
        ::memcpy(data, _section->content() + _next_byte, length);  // Flawfinder: ignore: memcpy()
        // Advance pointers
        data += length;
        remain_in_packet -= length;
        remain_in_section -= length;
        _next_byte += length;
        // If end of current section reached...
        if (remain_in_section == 0) {
            // Count sections
            _section_out_count++;
            // Remember next section if known
            _section = next_section;
            _next_byte = 0;
            next_section.clear();
            // If stuffing required at the end of packet, don't use next section
            if (do_stuffing) {
                break;
            }
            // If next section unknown, get it now
            if (_section.isNull()) {
                // If stuffing required before this section, give up
                if (_provider == nullptr || _provider->doStuffing()) {
                    break;
                }
                _provider->provideSection (_section_in_count++, _section);
                // If no next section, stuff the end of packet
                if (_section.isNull()) {
                    break;
                }
            }
            // We no longer know about stuffing after current section
            do_stuffing = false;
            // If no room for new section header, stuff the end of packet
            if (remain_in_packet < _section->headerSize()) {
                break;
            }
            // Get characteristcs of new section.
            remain_in_section = _section->size();
        }
    }

    // Do packet stuffing if necessary.
    ::memset (data, 0xFF, remain_in_packet);
    return true;
}


//----------------------------------------------------------------------------
// Display the internal state of the packetizer, mainly for debug
//----------------------------------------------------------------------------

std::ostream& ts::Packetizer::display(std::ostream& strm) const
{
    return strm
        << UString::Format(u"  PID: %d (0x%X)", {_pid, _pid}) << std::endl
        << "  Next CC: " << int(_continuity) << std::endl
        << "  Current section: "
        << (_section.isNull() ? UString(u"none") : UString::Format(u"%s, offset %d", {names::TID(_section->tableId()), _next_byte}))
        << std::endl
        << UString::Format(u"  Output packets: %'d", {_packet_count}) << std::endl
        << UString::Format(u"  Output sections: %'d", {_section_out_count}) << std::endl
        << UString::Format(u"  Provided sections: %'d", {_section_in_count}) << std::endl;
}
