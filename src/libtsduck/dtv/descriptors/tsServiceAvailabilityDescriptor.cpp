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

#include "tsServiceAvailabilityDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"service_availability_descriptor"
#define MY_DID ts::DID_SERVICE_AVAIL
#define MY_STD ts::STD_DVB

TS_XML_DESCRIPTOR_FACTORY(ts::ServiceAvailabilityDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::ServiceAvailabilityDescriptor, ts::EDID::Standard(MY_DID));
TS_FACTORY_REGISTER(ts::ServiceAvailabilityDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ServiceAvailabilityDescriptor::ServiceAvailabilityDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    availability(false),
    cell_ids()
{
    _is_valid = true;
}

ts::ServiceAvailabilityDescriptor::ServiceAvailabilityDescriptor(DuckContext& duck, const Descriptor& desc) :
    ServiceAvailabilityDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceAvailabilityDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(availability ? 0xFF : 0x7F);
    for (auto it = cell_ids.begin(); it != cell_ids.end(); ++it) {
        bbp->appendUInt16(*it);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ServiceAvailabilityDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() % 2 == 1;
    cell_ids.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        availability = (data[0] & 0x80) != 0;
        data++; size--;
        while (size >= 2) {
            cell_ids.push_back(GetUInt16(data));
            data += 2; size -= 2;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceAvailabilityDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        strm << margin << "Availability: " << UString::TrueFalse((data[0] & 0x80) != 0) << std::endl;
        data++; size--;
        while (size >= 2) {
            const uint16_t id = GetUInt16(data);
            data += 2; size -= 2;
            strm << margin << UString::Format(u"Cell id: 0x%X (%d)", {id, id}) << std::endl;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ServiceAvailabilityDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"availability", availability);
    for (auto it = cell_ids.begin(); it != cell_ids.end(); ++it) {
        root->addElement(u"cell")->setIntAttribute(u"id", *it, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::ServiceAvailabilityDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    cell_ids.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getBoolAttribute(availability, u"availability", true) &&
        element->getChildren(children, u"cell", 0, MAX_CELLS);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        uint16_t id = 0;
        _is_valid = children[i]->getIntAttribute<uint16_t>(id, u"id", true);
        if (_is_valid) {
            cell_ids.push_back(id);
        }
    }
}
