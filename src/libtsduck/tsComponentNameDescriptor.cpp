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

#include "tsComponentNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"component_name_descriptor"
#define MY_DID ts::DID_ATSC_COMPONENT_NAME
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::STD_ATSC

TS_XML_DESCRIPTOR_FACTORY(ts::ComponentNameDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::ComponentNameDescriptor, ts::EDID::Private(MY_DID, MY_PDS));
TS_FACTORY_REGISTER(ts::ComponentNameDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, MY_PDS));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::ComponentNameDescriptor::ComponentNameDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    component_name_string()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::ComponentNameDescriptor::ComponentNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    ComponentNameDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ComponentNameDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    component_name_string.serialize(duck, *bbp);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ComponentNameDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    component_name_string.clear();
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == _tag && component_name_string.deserialize(duck, data, size);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ComponentNameDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    ATSCMultipleString::Display(display, u"Component name: ", indent, data, size);
    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ComponentNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    component_name_string.toXML(duck, root, u"component_name_string", true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::ComponentNameDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        component_name_string.fromXML(duck, element, u"component_name_string", false);
}
