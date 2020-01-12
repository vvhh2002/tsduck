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

#include "tsCaptionServiceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"caption_service_descriptor"
#define MY_DID ts::DID_ATSC_CAPTION
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::STD_ATSC

TS_XML_DESCRIPTOR_FACTORY(ts::CaptionServiceDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::CaptionServiceDescriptor, ts::EDID::Private(MY_DID, MY_PDS));
TS_FACTORY_REGISTER(ts::CaptionServiceDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, MY_PDS));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CaptionServiceDescriptor::CaptionServiceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
{
    _is_valid = true;
}

ts::CaptionServiceDescriptor::CaptionServiceDescriptor(DuckContext& duck, const Descriptor& desc) :
    CaptionServiceDescriptor()
{
    deserialize(duck, desc);
}

ts::CaptionServiceDescriptor::Entry::Entry() :
    language(),
    digital_cc(false),
    line21_field(false),
    caption_service_number(0),
    easy_reader(false),
    wide_aspect_ratio(false)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CaptionServiceDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(0xE0 | (entries.size() & 0x1F)));
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (!SerializeLanguageCode(*bbp, it->language)) {
            return;
        }
        if (it->digital_cc) {
            bbp->appendUInt8(uint8_t(0xC0 | (it->caption_service_number & 0x3F)));
        }
        else {
            bbp->appendUInt8(it->line21_field ? 0x7F : 0x7E);
        }
        bbp->appendUInt16((it->easy_reader ? 0x8000 : 0x0000) | (it->wide_aspect_ratio ? 0x4000 : 0x0000) | 0x3FFF);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CaptionServiceDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 1;

    if (_is_valid) {
        size_t count = data[0] & 0x1F;
        data++; size--;
        while (size >= 6 && count > 0) {
            Entry e;
            e.language = DeserializeLanguageCode(data);
            e.digital_cc = (data[3] & 0x80) != 0;
            if (e.digital_cc) {
                e.caption_service_number = data[3] & 0x3F;
            }
            else {
                e.line21_field = (data[3] & 0x01) != 0;
            }
            e.easy_reader = (data[4] & 0x80) != 0;
            e.wide_aspect_ratio = (data[4] & 0x40) != 0;
            entries.push_back(e);
            data += 6; size -= 6; count--;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CaptionServiceDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        size_t count = data[0] & 0x1F;
        data++; size--;
        strm << margin << "Number of services: " << count << std::endl;
        while (size >= 6 && count > 0) {
            const bool digital = (data[3] & 0x80) != 0;
            strm << margin << UString::Format(u"- Language: \"%s\", digital: %s", {DeserializeLanguageCode(data), digital});
            if (digital) {
                const uint8_t id = data[3] & 0x3F;
                strm << UString::Format(u", service: 0x%X (%d)", {id, id});
            }
            else {
                strm << UString::Format(u", line 21: %s", {(data[3] & 0x01) != 0});
            }
            strm << UString::Format(u", easy reader: %s, wide: %s", {(data[4] & 0x80) != 0, (data[4] & 0x40) != 0}) << std::endl;
            data += 6; size -= 6; count--;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CaptionServiceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"service");
        e->setAttribute(u"language", it->language);
        e->setBoolAttribute(u"digital_cc", it->digital_cc);
        if (it->digital_cc) {
            e->setIntAttribute(u"caption_service_number", it->caption_service_number, true);
        }
        else {
            e->setBoolAttribute(u"line21_field", it->line21_field);
        }
        e->setBoolAttribute(u"easy_reader", it->easy_reader);
        e->setBoolAttribute(u"wide_aspect_ratio", it->wide_aspect_ratio);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::CaptionServiceDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    entries.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        Entry entry;
        _is_valid =
            children[i]->getAttribute(entry.language, u"language", true, UString(), 3, 3) &&
            children[i]->getBoolAttribute(entry.digital_cc, u"digital_cc", true) &&
            children[i]->getBoolAttribute(entry.line21_field, u"line21_field", false) &&
            children[i]->getIntAttribute<uint8_t>(entry.caption_service_number, u"caption_service_number", false, 0, 0, 0x3F) &&
            children[i]->getBoolAttribute(entry.easy_reader, u"easy_reader", true) &&
            children[i]->getBoolAttribute(entry.wide_aspect_ratio, u"wide_aspect_ratio", true);
        if (_is_valid) {
            entries.push_back(entry);
        }
    }
}
