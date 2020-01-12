//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsScramblingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"scrambling_descriptor"
#define MY_DID ts::DID_SCRAMBLING
#define MY_STD ts::STD_DVB

TS_XML_DESCRIPTOR_FACTORY(ts::ScramblingDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::ScramblingDescriptor, ts::EDID::Standard(MY_DID));
TS_FACTORY_REGISTER(ts::ScramblingDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ScramblingDescriptor::ScramblingDescriptor(uint8_t mode) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    scrambling_mode(mode)
{
    _is_valid = true;
}

ts::ScramblingDescriptor::ScramblingDescriptor(DuckContext& duck, const Descriptor& desc) :
    ScramblingDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ScramblingDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(scrambling_mode);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ScramblingDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() == 1;

    if (_is_valid) {
        scrambling_mode = GetUInt8(desc.payload());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ScramblingDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t mode = data[0];
        data += 1; size -= 1;
        strm << margin << UString::Format(u"Scrambling mode: %s", {NameFromSection(u"ScramblingMode", mode, names::HEXA_FIRST)}) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ScramblingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"scrambling_mode", scrambling_mode, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::ScramblingDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(scrambling_mode, u"scrambling_mode", true);
}
