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

#include "tsMPEG2AACAudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"MPEG2_AAC_audio_descriptor"
#define MY_DID ts::DID_MPEG2_AAC_AUDIO
#define MY_STD ts::STD_MPEG

TS_XML_DESCRIPTOR_FACTORY(ts::MPEG2AACAudioDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::MPEG2AACAudioDescriptor, ts::EDID::Standard(MY_DID));
TS_FACTORY_REGISTER(ts::MPEG2AACAudioDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEG2AACAudioDescriptor::MPEG2AACAudioDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    MPEG2_AAC_profile(0),
    MPEG2_AAC_channel_configuration(0),
    MPEG2_AAC_additional_information(0)
{
    _is_valid = true;
}

ts::MPEG2AACAudioDescriptor::MPEG2AACAudioDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEG2AACAudioDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEG2AACAudioDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MPEG2_AAC_profile);
    bbp->appendUInt8(MPEG2_AAC_channel_configuration);
    bbp->appendUInt8(MPEG2_AAC_additional_information);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MPEG2AACAudioDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size == 3;

    if (_is_valid) {
        MPEG2_AAC_profile = data[0];
        MPEG2_AAC_channel_configuration = data[1];
        MPEG2_AAC_additional_information = data[2];
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEG2AACAudioDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');

    if (size >= 3) {
        strm << margin << UString::Format(u"MPEG-2 AAC profile: 0x%X (%d)", {data[0], data[0]}) << std::endl
             << margin << UString::Format(u"MPEG-2 AAC channel configuration: 0x%X (%d)", {data[1], data[1]}) << std::endl
             << margin << UString::Format(u"MPEG-2 AAC additional information: 0x%X (%d)", {data[2], data[2]}) << std::endl;
        data += 3; size -= 3;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEG2AACAudioDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"MPEG2_AAC_profile", MPEG2_AAC_profile, true);
    root->setIntAttribute(u"MPEG2_AAC_channel_configuration", MPEG2_AAC_channel_configuration, true);
    root->setIntAttribute(u"MPEG2_AAC_additional_information", MPEG2_AAC_additional_information, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::MPEG2AACAudioDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(MPEG2_AAC_profile, u"MPEG2_AAC_profile", true) &&
        element->getIntAttribute<uint8_t>(MPEG2_AAC_channel_configuration, u"MPEG2_AAC_channel_configuration", true) &&
        element->getIntAttribute<uint8_t>(MPEG2_AAC_additional_information, u"MPEG2_AAC_additional_information", true);
}
