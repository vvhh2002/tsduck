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

#include "tsDVBHTMLApplicationLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"dvb_html_application_location_descriptor"
#define MY_DID ts::DID_AIT_HTML_APP_LOC
#define MY_TID ts::TID_AIT
#define MY_STD ts::STD_DVB

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::DVBHTMLApplicationLocationDescriptor, MY_XML_NAME, MY_TID);
TS_ID_DESCRIPTOR_FACTORY(ts::DVBHTMLApplicationLocationDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));
TS_FACTORY_REGISTER(ts::DVBHTMLApplicationLocationDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::DVBHTMLApplicationLocationDescriptor::DVBHTMLApplicationLocationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    physical_root(),
    initial_path()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::DVBHTMLApplicationLocationDescriptor::DVBHTMLApplicationLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBHTMLApplicationLocationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationLocationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->append(duck.toDVBWithByteLength(physical_root));
    bbp->append(duck.toDVB(initial_path));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationLocationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    physical_root.clear();
    initial_path.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 1;

    if (_is_valid) {
        const size_t len = data[0];
        _is_valid = len + 1 <= size;
        if (_is_valid) {
            physical_root = duck.fromDVB(data + 1, len);
            initial_path = duck.fromDVB(data + 1 + len, size - len - 1);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationLocationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        size_t len = std::min<size_t>(data[0], size - 1);
        strm << margin << "Physical root: \"" << display.duck().fromDVB(data + 1, len) << "\"" << std::endl
             << margin << "Initial path: \"" << display.duck().fromDVB(data + 1 + len, size - len - 1) << "\"" << std::endl;
        size = 0;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"physical_root", physical_root);
    root->setAttribute(u"initial_path", initial_path);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationLocationDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getAttribute(physical_root, u"physical_root", true) &&
        element->getAttribute(initial_path, u"initial_path", true);
}
