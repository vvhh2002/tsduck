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
//  Representation of a Time & Date Table (TDT)
//
//----------------------------------------------------------------------------

#include "tsTDT.h"
#include "tsMJD.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"TDT"
#define MY_TID ts::TID_TDT
#define MY_STD ts::STD_DVB

TS_XML_TABLE_FACTORY(ts::TDT, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::TDT, MY_TID, MY_STD);
TS_FACTORY_REGISTER(ts::TDT::DisplaySection, MY_TID);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TDT::TDT(const Time& utc_time_) :
    AbstractTable(MY_TID, MY_XML_NAME, MY_STD),
    utc_time(utc_time_)
{
    _is_valid = true;
}

ts::TDT::TDT(DuckContext& duck, const BinaryTable& table) :
    TDT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TDT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // This is a short table, must have only one section
    if (table.sectionCount() != 1) {
        return;
    }

    // Reference to single section
    const Section& sect(*table.sectionAt(0));

    // Get UTC time.
    if (sect.payloadSize() >= MJD_SIZE) {
        DecodeMJD(sect.payload(), MJD_SIZE, utc_time);
        _is_valid = true;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TDT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Encode the data in MJD in the payload (5 bytes)
    uint8_t payload[MJD_SIZE];
    EncodeMJD(utc_time, payload, MJD_SIZE);

    // Add the section in the table
    table.addSection(new Section(MY_TID, // tid
                                 true,    // is_private_section
                                 payload,
                                 MJD_SIZE));
}


//----------------------------------------------------------------------------
// A static method to display a TDT section.
//----------------------------------------------------------------------------

void ts::TDT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.duck().out());
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 5) {
        Time time;
        DecodeMJD(data, 5, time);
        data += 5; size -= 5;
        strm << std::string(indent, ' ') << "UTC time: "
             << time.format(Time::DATE | Time::TIME) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TDT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setDateTimeAttribute(u"UTC_time", utc_time);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::TDT::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getDateTimeAttribute(utc_time, u"UTC_time", true);
}
