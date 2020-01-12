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

#include "tsDVBCharset.h"
#include "tsAlgorithm.h"
#include "tsSingletonManager.h"
#include "tsByteBlock.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr uint8_t  ts::DVBCharset::DVB_SINGLE_BYTE_CRLF;
constexpr uint16_t ts::DVBCharset::DVB_CODEPOINT_CRLF;
#endif


//----------------------------------------------------------------------------
// Get the character coding table at the beginning of a DVB string.
//----------------------------------------------------------------------------

bool ts::DVBCharset::GetCharCodeTable(uint32_t& code, size_t& codeSize, const uint8_t* dvb, size_t dvbSize)
{
    // Null or empty buffer is a valid empty string.
    if (dvb == nullptr || dvbSize == 0) {
        code = 0;
        codeSize = 0;
        return true;
    }
    else if (*dvb >= 0x20) {
        // Default character set.
        code = 0;
        codeSize = 0;
        return true;
    }
    else if (*dvb == 0x1F) {
        if (dvbSize >= 2) {
            // Second byte is encoding_type_id.
            // Currently unsupported.
            code = 0xFFFFFFFF;
            codeSize = 2;
            return false;
        }
    }
    else if (*dvb == 0x10) {
        if (dvbSize >= 3) {
            code = GetUInt24(dvb);
            codeSize = 3;
            return true;
        }
    }
    else {
        code = *dvb;
        codeSize = 1;
        return true;
    }

    // Invalid format
    code = 0xFFFFFFFF;
    codeSize = 0;
    return false;
}


//----------------------------------------------------------------------------
// Encode the character set table code.
//----------------------------------------------------------------------------

size_t ts::DVBCharset::encodeTableCode(uint8_t*& buffer, size_t& size) const
{
    // Intermediate buffer, just in case the output buffer is too small.
    uint8_t buf[4] = {0};
    size_t codeSize = 0;

    if (buffer == nullptr || size == 0 || _code == 0) {
        // Empty buffer or default character set.
        return 0;
    }
    else if (_code < 0x1F && _code != 0x10) {
        // On byte code.
        buf[0] = uint8_t(_code);
        codeSize = 1;
    }
    else if ((_code & 0xFFFFFF00) == 0x00001F00) {
        // Two bytes, 0x1F followed by encoding_type_id.
        PutUInt16(buf, uint16_t(_code));
        codeSize = 2;
    }
    else if ((_code & 0xFFFF0000) == 0x00100000) {
        // Three bytes, 0x10 followed by 16-bit code.
        PutUInt24(buf, _code);
        codeSize = 3;
    }
    else {
        // Invalid table code.
        return 0;
    }

    // Now copy the table code.
    if (codeSize > size) {
        codeSize = size;
    }
    ::memcpy(buffer, buf, codeSize);
    buffer += codeSize;
    size -= codeSize;
    return codeSize;
}


//----------------------------------------------------------------------------
// Encode a C++ Unicode string into a DVB string as a ByteBlock.
//----------------------------------------------------------------------------

ts::ByteBlock ts::DVBCharset::encoded(const UString& str, size_t start, size_t count) const
{
    // The maximum number of DVB bytes per character is 4 (worst case in UTF-8).
    ByteBlock bb(UString::UTF8_CHAR_MAX_SIZE * std::min(str.length() - start, count));

    // Convert the string.
    uint8_t* buffer = bb.data();
    size_t size = bb.size();
    encode(buffer, size, str, start, count);

    // Truncate unused bytes.
    assert(size <= bb.size());
    bb.resize(bb.size() - size);
    return bb;
}


//----------------------------------------------------------------------------
// Repository of character sets.
//----------------------------------------------------------------------------

namespace {
    class CharSetRepo
    {
        TS_DECLARE_SINGLETON(CharSetRepo);
    public:
        std::map<ts::UString, ts::DVBCharset*> byName;
        std::map<uint32_t,   ts::DVBCharset*> byCode;
    };
    TS_DEFINE_SINGLETON(CharSetRepo);
    CharSetRepo::CharSetRepo() : byName(), byCode() {}
}

// Get a DVB character set by name.
ts::DVBCharset* ts::DVBCharset::GetCharset(const UString& name)
{
    const CharSetRepo* repo = CharSetRepo::Instance();
    const std::map<UString, DVBCharset*>::const_iterator it = repo->byName.find(name);
    return it == repo->byName.end() ? nullptr : it->second;
}

// Get a DVB character set by table code.
ts::DVBCharset* ts::DVBCharset::GetCharset(uint32_t tableCode)
{
    const CharSetRepo* repo = CharSetRepo::Instance();
    const std::map<uint32_t, DVBCharset*>::const_iterator it = repo->byCode.find(tableCode);
    return it == repo->byCode.end() ? nullptr : it->second;
}

// Find all registered character set names.
ts::UStringList ts::DVBCharset::GetAllNames()
{
    return MapKeys(CharSetRepo::Instance()->byName);
}

// Remove the specified charset
void ts::DVBCharset::Unregister(const DVBCharset* charset)
{
    if (charset != nullptr) {
        CharSetRepo* repo = CharSetRepo::Instance();
        repo->byName.erase(charset->name());
        repo->byCode.erase(charset->tableCode());
    }
}


//----------------------------------------------------------------------------
// Constructor / destructor.
//----------------------------------------------------------------------------

ts::DVBCharset::DVBCharset(const UString& name, uint32_t tableCode) :
    _name(name),
    _code(tableCode)
{
    // Register the character set.
    CharSetRepo* repo = CharSetRepo::Instance();
    const std::map<UString, DVBCharset*>::const_iterator itName = repo->byName.find(_name);
    const std::map<uint32_t, DVBCharset*>::const_iterator itCode = repo->byCode.find(_code);
    if (itName == repo->byName.end() && itCode == repo->byCode.end()) {
        // Charset not yet registered.
        repo->byName.insert(std::make_pair(_name, this));
        repo->byCode.insert(std::make_pair(_code, this));
    }
    else {
        throw DuplicateDVBCharset(_name);
    }
}

ts::DVBCharset::~DVBCharset()
{
    // Remove charset from repository.
    Unregister(this);
}
