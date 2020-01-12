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

#include "tsMVCExtensionDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"MVC_extension_descriptor"
#define MY_DID ts::DID_MVC_EXT
#define MY_STD ts::STD_MPEG

TS_XML_DESCRIPTOR_FACTORY(ts::MVCExtensionDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::MVCExtensionDescriptor, ts::EDID::Standard(MY_DID));
TS_FACTORY_REGISTER(ts::MVCExtensionDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MVCExtensionDescriptor::MVCExtensionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    average_bitrate(0),
    maximum_bitrate(0),
    view_association_not_present(false),
    base_view_is_left_eyeview(false),
    view_order_index_min(0),
    view_order_index_max(0),
    temporal_id_start(0),
    temporal_id_end(0),
    no_sei_nal_unit_present(false),
    no_prefix_nal_unit_present(false)
{
    _is_valid = true;
}

ts::MVCExtensionDescriptor::MVCExtensionDescriptor(DuckContext& duck, const Descriptor& desc) :
    MVCExtensionDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MVCExtensionDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(average_bitrate);
    bbp->appendUInt16(maximum_bitrate);
    bbp->appendUInt24((view_association_not_present ? 0x800000 : 0x000000) |
                      (base_view_is_left_eyeview ? 0x700000 : 0x300000) |
                      (uint32_t(view_order_index_min & 0x03FF) << 10) |
                      (view_order_index_max & 0x03FF));
    bbp->appendUInt8(uint8_t(temporal_id_start << 5) |
                     uint8_t((temporal_id_end & 0x07) << 2) |
                     (no_sei_nal_unit_present ? 0x02 : 0x00) |
                     (no_prefix_nal_unit_present ? 0x01 : 0x00));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MVCExtensionDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size == 8;

    if (_is_valid) {
        average_bitrate = GetUInt16(data);
        maximum_bitrate = GetUInt16(data + 2);
        const uint32_t val = GetUInt24(data + 4);
        view_association_not_present = (val & 0x800000) != 0;
        base_view_is_left_eyeview = (val & 0x400000) != 0;
        view_order_index_min = uint16_t((val >> 10) & 0x03FF);
        view_order_index_max = uint16_t(val & 0x03FF);
        temporal_id_start = (data[7] >> 5) & 0x07;
        temporal_id_end = (data[7] >> 2) & 0x07;
        no_sei_nal_unit_present = (data[7] & 0x02) != 0;
        no_prefix_nal_unit_present = (data[7] & 0x01) != 0;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MVCExtensionDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');

    if (size >= 8) {
        const uint32_t val = GetUInt24(data + 4);
        strm << margin << UString::Format(u"Average bitrate: %d kb/s, maximum: %d kb/s", {GetUInt16(data), GetUInt16(data + 2)}) << std::endl
             << margin << UString::Format(u"View association not present: %s", {(val & 0x800000) != 0}) << std::endl
             << margin << UString::Format(u"Base view is left eyeview: %s", {(val & 0x400000) != 0}) << std::endl
             << margin << UString::Format(u"View order min: %d, max: %d", {(val >> 10) & 0x03FF, val & 0x03FF}) << std::endl
             << margin << UString::Format(u"Temporal id start: %d, end: %d", {(data[7] >> 5) & 0x07, (data[7] >> 2) & 0x07}) << std::endl
             << margin << UString::Format(u"No SEI NALunit present: %s", {(data[7] & 0x02) != 0}) << std::endl
             << margin << UString::Format(u"No prefix NALunit present: %s", {(data[7] & 0x01) != 0}) << std::endl;
        data += 8; size -= 8;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MVCExtensionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"average_bitrate", average_bitrate);
    root->setIntAttribute(u"maximum_bitrate", maximum_bitrate);
    root->setBoolAttribute(u"view_association_not_present", view_association_not_present);
    root->setBoolAttribute(u"base_view_is_left_eyeview", base_view_is_left_eyeview);
    root->setIntAttribute(u"view_order_index_min", view_order_index_min);
    root->setIntAttribute(u"view_order_index_max", view_order_index_max);
    root->setIntAttribute(u"temporal_id_start", temporal_id_start);
    root->setIntAttribute(u"temporal_id_end", temporal_id_end);
    root->setBoolAttribute(u"no_sei_nal_unit_present", no_sei_nal_unit_present);
    root->setBoolAttribute(u"no_prefix_nal_unit_present", no_prefix_nal_unit_present);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::MVCExtensionDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint16_t>(average_bitrate, u"average_bitrate", true) &&
        element->getIntAttribute<uint16_t>(maximum_bitrate, u"maximum_bitrate", true) &&
        element->getBoolAttribute(view_association_not_present, u"view_association_not_present", true) &&
        element->getBoolAttribute(base_view_is_left_eyeview, u"base_view_is_left_eyeview", true) &&
        element->getIntAttribute<uint16_t>(view_order_index_min, u"view_order_index_min", true, 0, 0x0000, 0x03FF) &&
        element->getIntAttribute<uint16_t>(view_order_index_max, u"view_order_index_max", true, 0, 0x0000, 0x03FF) &&
        element->getIntAttribute<uint8_t>(temporal_id_start, u"temporal_id_start", true, 0, 0x00, 0x07) &&
        element->getIntAttribute<uint8_t>(temporal_id_end, u"temporal_id_end", true, 0, 0x00, 0x07) &&
        element->getBoolAttribute(no_sei_nal_unit_present, u"no_sei_nal_unit_present", true) &&
        element->getBoolAttribute(no_prefix_nal_unit_present, u"no_prefix_nal_unit_present", true);
}
