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

#include "tsDCCDepartingRequestDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"dcc_departing_request_descriptor"
#define MY_DID ts::DID_ATSC_DCC_DEPARTING
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::STD_ATSC

TS_XML_DESCRIPTOR_FACTORY(ts::DCCDepartingRequestDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::DCCDepartingRequestDescriptor, ts::EDID::Private(MY_DID, MY_PDS));
TS_FACTORY_REGISTER(ts::DCCDepartingRequestDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, MY_PDS));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DCCDepartingRequestDescriptor::DCCDepartingRequestDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    dcc_departing_request_type(0),
    dcc_departing_request_text()
{
    _is_valid = true;
}

ts::DCCDepartingRequestDescriptor::DCCDepartingRequestDescriptor(DuckContext& duck, const Descriptor& desc) :
    DCCDepartingRequestDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DCCDepartingRequestDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(dcc_departing_request_type);
    dcc_departing_request_text.lengthSerialize(duck, *bbp);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DCCDepartingRequestDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    dcc_departing_request_text.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 2;

    if (_is_valid) {
        dcc_departing_request_type = *data++;
        size--;
        _is_valid = dcc_departing_request_text.lengthDeserialize(duck, data, size);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DCCDepartingRequestDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    if (size >= 2) {
        display.duck().out() << UString::Format(u"%*sDCC departing request type: 0x%X (%d)", {indent, u"", data[0], data[0]}) << std::endl;
        const size_t len = data[1];
        data += 2; size -= 2;
        ATSCMultipleString::Display(display, u"DCC departing request text: ", indent, data, size, len);
    }
    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DCCDepartingRequestDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"dcc_departing_request_type", dcc_departing_request_type, true);
    dcc_departing_request_text.toXML(duck, root, u"dcc_departing_request_text", true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::DCCDepartingRequestDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(dcc_departing_request_type, u"dcc_departing_request_type", true) &&
        dcc_departing_request_text.fromXML(duck, element, u"dcc_departing_request_text", false);
}
