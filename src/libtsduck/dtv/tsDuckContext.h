//-------------------       ---------------------------------------------------------
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
//!
//!  @file
//!  TSDuck execution context containing current preferences.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsByteBlock.h"
#include "tsMPEG.h"

namespace ts {

    class DVBCharset;
    class HFBand;
    class Report;
    class Args;

    //!
    //! TSDuck execution context containing current preferences.
    //! @ingroup app
    //!
    //! An instance of this class contains specific contextual information
    //! for the execution of TSDuck. This context contains either user's
    //! preferences and accumulated contextual information.
    //!
    //! Context information include:
    //! - Report for log and error messages.
    //! - Text output stream.
    //! - Default DVB character sets (input and output).
    //! - Default CAS id.
    //! - Default Private Data Specifier (PDS) for DVB private descriptors.
    //! - Accumulated standards from the signalization (MPEG, DVB, ATSC, etc.)
    //! - Default region for UHF and VHF frequency layout.
    //!
    //! Support is included to define and analyze command line options which
    //! define values for the environment.
    //!
    //! Unlike DuckConfigFile, this class is not a singleton. More than
    //! one context is allowed in the same process as long as the various
    //! instances of classes which use DuckContext use only one context at
    //! a time. For instance, inside a @e tsp or @e tsswitch process, each
    //! plugin can use its own context, using different preferences.
    //!
    //! The class DuckContext is not thread-safe. It shall be used from one
    //! single thread or explicit synchronization is required.
    //!
    class TSDUCKDLL DuckContext
    {
        TS_NOCOPY(DuckContext);
    public:
        //!
        //! Constructor.
        //! @param [in] report Address of the report for log and error messages. If null, use the standard error.
        //! @param [in] output The output stream to use, @c std::cout on null pointer.
        //!
        DuckContext(Report* report = nullptr, std::ostream* output = nullptr);

        //!
        //! Reset the TSDuck context to initial configuration.
        //!
        void reset();

        //!
        //! Get the current report for log and error messages.
        //! @return A reference to the current output report.
        //!
        Report& report() const { return *_report; }

        //!
        //! Set a new report for log and error messages.
        //! @param [in] report Address of the report for log and error messages. If null, use the standard error.
        //!
        void setReport(Report* report);

        //!
        //! Get the current output stream to issue long text output.
        //! @return A reference to the output stream.
        //!
        std::ostream& out() const { return *_out; }

        //!
        //! Redirect the output stream to a file.
        //! @param [in] fileName The file name to create. If empty, reset to @c std::cout.
        //! @param [in] override It true, the previous file is closed. If false and the
        //! output is already redirected outside @c std::cout, do nothing.
        //! @return True on success, false on error.
        //!
        bool setOutput(const UString& fileName, bool override = true);

        //!
        //! Redirect the output stream to a stream.
        //! @param [in] output The output stream to use, @c std::cout on null pointer.
        //! @param [in] override It true, the previous file is closed. If false and the
        //! output is already redirected outside @c std::cout, do nothing.
        //!
        void setOutput(std::ostream* output, bool override = true);

        //!
        //! Flush the text output.
        //!
        void flush();

        //!
        //! A utility method to interpret data as an ASCII string.
        //! @param [in] data Address of data.
        //! @param [in] size Size of data.
        //! @return If all bytes in data are ASCII (optionally padded with zeroes), return the
        //! equivalent ASCII string. Otherwise, return an empty string.
        //!
        std::string toASCII(const void *data, size_t size) const;

        //!
        //! A utility method to display data if it can be interpreted as an ASCII string.
        //! @param [in] data Address of data.
        //! @param [in] size Size of data.
        //! @param [in] prefix To print before the ASCII data.
        //! @param [in] suffix To print after the ASCII data.
        //! @return A reference to the output stream.
        //!
        std::ostream& displayIfASCII(const void *data, size_t size, const UString& prefix = UString(), const UString& suffix = UString());

        //!
        //! Get the default input DVB character set for DVB strings without table code.
        //! The default is the DVB superset of ISO/IEC 6937 as defined in ETSI EN 300 468.
        //! Other defaults can be used in the context of an operator using an incorrect
        //! signalization, assuming another default character set (usually from its own country).
        //! @return The default input DVB character set or the null pointer if none is defined.
        //!
        const DVBCharset* dvbCharsetIn() const { return _dvbCharsetIn; }

        //!
        //! Get the preferred output DVB character set for DVB strings.
        //! @return The preferred output DVB character set or the null pointer if none is defined.
        //!
        const DVBCharset* dvbCharsetOut() const { return _dvbCharsetOut; }

        //!
        //! Convert a DVB string into UTF-16 using the default input DVB character set.
        //! @param [in] dvb A string in DVB representation.
        //! The first bytes of the string indicate the DVB character set to use.
        //! @return The equivalent UTF-16 string. Stop on untranslatable character, if any.
        //! @see ETSI EN 300 468, Annex A.
        //!
        UString fromDVB(const std::string& dvb) const
        {
            return UString::FromDVB(dvb, _dvbCharsetIn);
        }

        //!
        //! Convert a DVB string into UTF-16 using the default input DVB character set.
        //! @param [in] dvb Address of a string in DVB representation.
        //! The first bytes of the string indicate the DVB character set to use.
        //! @param [in] dvbSize Size in bytes of the DVB string.
        //! @return The equivalent UTF-16 string. Stop on untranslatable character, if any.
        //! @see ETSI EN 300 468, Annex A.
        //!
        UString fromDVB(const uint8_t* dvb, size_t dvbSize) const
        {
            return UString::FromDVB(dvb, dvbSize, _dvbCharsetIn);
        }

        //!
        //! Convert a DVB string into UTF-16 (preceded by its one-byte length) using the default input DVB character set.
        //! @param [in,out] buffer Address of a buffer containing a DVB string to read.
        //! The first byte in the buffer is the length in bytes of the string.
        //! Upon return, @a buffer is updated to point after the end of the string.
        //! @param [in,out] size Size in bytes of the buffer, which may be larger than
        //! the DVB string. Upon return, @a size is updated, decremented by the same amount
        //! @a buffer was incremented.
        //! @return The equivalent UTF-16 string. Stop on untranslatable character, if any.
        //! @see ETSI EN 300 468, Annex A.
        //!
        UString fromDVBWithByteLength(const uint8_t*& buffer, size_t& size) const
        {
            return UString::FromDVBWithByteLength(buffer, size, _dvbCharsetIn);
        }

        //!
        //! Encode an UTF-16 string into a DVB string using the preferred output DVB character set.
        //! Stop either when this string is serialized or when the buffer is full, whichever comes first.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in,out] buffer Address of the buffer where the DVB string is written.
        //! The address is updated to point after the encoded value.
        //! @param [in,out] size Size of the buffer. Updated to remaining size.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_t toDVB(const UString& str, uint8_t*& buffer, size_t& size, size_t start = 0, size_t count = NPOS) const
        {
            return str.toDVB(buffer, size, start, count, _dvbCharsetOut);
        }

        //!
        //! Encode an UTF-16 string into a DVB string using the preferred output DVB character set.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return The DVB string.
        //!
        ByteBlock toDVB(const UString& str, size_t start = 0, size_t count = NPOS) const
        {
            return str.toDVB(start, count, _dvbCharsetOut);
        }

        //!
        //! Encode an UTF-16 string into a DVB string (preceded by its one-byte length) using the preferred output DVB character set.
        //! Stop either when this string is serialized or when the buffer is full or when 255 bytes are written, whichever comes first.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in,out] buffer Address of the buffer where the DVB string is written.
        //! The first byte will receive the size in bytes of the DVB string.
        //! The address is updated to point after the encoded value.
        //! @param [in,out] size Size of the buffer. Updated to remaining size.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_t toDVBWithByteLength(const UString& str, uint8_t*& buffer, size_t& size, size_t start = 0, size_t count = NPOS) const
        {
            return str.toDVBWithByteLength(buffer, size, start, count, _dvbCharsetOut);
        }

        //!
        //! Encode an UTF-16 string into a DVB string (preceded by its one-byte length) using the preferred output DVB character set.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return The DVB string with the initial length byte.
        //!
        ByteBlock toDVBWithByteLength(const UString& str, size_t start = 0, size_t count = NPOS) const
        {
            return str.toDVBWithByteLength(start, count, _dvbCharsetOut);
        }

        //!
        //! Set the default input DVB character set for DVB strings without table code.
        //! The default should be the DVB superset of ISO/IEC 6937 as defined in ETSI EN 300 468.
        //! Use another default in the context of an operator using an incorrect signalization,
        //! assuming another default character set (usually from its own country).
        //! @param [in] charset The new default input DVB character set or a null pointer to revert
        //! to the default.
        //!
        void setDefaultDVBCharsetIn(const DVBCharset* charset);

        //!
        //! Set the preferred output DVB character set for DVB strings.
        //! @param [in] charset The new preferred output DVB character set or a null pointer to revert
        //! to the default.
        //!
        void setDefaultDVBCharsetOut(const DVBCharset* charset);

        //!
        //! Set the default CAS id to use.
        //! @param [in] cas Default CAS id to be used when the CAS is unknown.
        //!
        void setDefaultCASId(uint16_t cas);

        //!
        //! The actual CAS id to use.
        //! @param [in] cas Proposed CAS id. If equal to CASID_NULL, then another value can be returned.
        //! @return The actual CAS id to use.
        //!
        uint16_t casId(uint16_t cas = CASID_NULL) const;

        //!
        //! Set the default private data specifier to use in the absence of explicit private_data_specifier_descriptor.
        //! @param [in] pds Default PDS. Use zero to revert to no default.
        //!
        void setDefaultPDS(PDS pds);

        //!
        //! The actual private data specifier to use.
        //! @param [in] pds Current PDS, typically from a private_data_specifier_descriptor.
        //! @return The actual PDS to use.
        //!
        PDS actualPDS(PDS pds) const;

        //!
        //! Get the list of standards which are present in the transport stream or context.
        //! @return A bit mask of standards.
        //!
        Standards standards() const { return _accStandards; }

        //!
        //! Add a list of standards which are present in the transport stream or context.
        //! @param [in] mask A bit mask of standards.
        //!
        void addStandards(Standards mask);

        //!
        //! Reset the list of standards which are present in the transport stream or context.
        //! @param [in] mask A bit mask of standards.
        //!
        void resetStandards(Standards mask = STD_NONE);

        //!
        //! Set the name of the default region for UVH and VHF band frequency layout.
        //! @param [in] region Name of the region. Use an empty string to revert to the default.
        //!
        void setDefaultHFRegion(const UString& region);

        //!
        //! Get the name of the default region for UVH and VHF band frequency layout.
        //! @return Name of the default region.
        //!
        UString defaultHFRegion() const;

        //!
        //! Get the description of the VHF band for the default region.
        //! @return The description of the VHF band for the default region. Never null.
        //!
        const HFBand* vhfBand() const;

        //!
        //! Get the description of the UHF band for the default region.
        //! @return The description of the UHF band for the default region. Never null.
        //!
        const HFBand* uhfBand() const;

        //!
        //! Define DVB character set command line options in an Args.
        //! Defined options: @c -\-default-charset, @c -\-europe.
        //! The context keeps track of defined options so that loadOptions() can parse the appropriate options.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgsForDVBCharset(Args& args) { defineOptions(args, CMD_DVB_CHARSET); }

        //!
        //! Define default CAS command line options in an Args.
        //! Defined options: @c -\-default-cas-id and other CAS-specific options.
        //! The context keeps track of defined options so that loadOptions() can parse the appropriate options.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgsForCAS(Args& args) { defineOptions(args, CMD_CAS); }

        //!
        //! Define Private Data Specifier command line options in an Args.
        //! Defined options: @c -\-default-pds.
        //! The context keeps track of defined options so that loadOptions() can parse the appropriate options.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgsForPDS(Args& args) { defineOptions(args, CMD_PDS); }

        //!
        //! Define contextual standards command line options in an Args.
        //! Defined options: @c -\-atsc.
        //! The context keeps track of defined options so that loadOptions() can parse the appropriate options.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgsForStandards(Args& args) { defineOptions(args, CMD_STANDARDS); }

        //!
        //! Define HF band command line options in an Args.
        //! Defined options: @c -\-hf-band-region.
        //! The context keeps track of defined options so that loadOptions() can parse the appropriate options.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgsForHFBand(Args& args) { defineOptions(args, CMD_HF_REGION); }

        //!
        //! Load the values of all previously defined arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(Args& args);

    private:
        Report*           _report;            // Pointer to a report for error messages. Never null.
        std::ostream*     _initial_out;       // Initial text output stream. Never null.
        std::ostream*     _out;               // Pointer to text output stream. Never null.
        std::ofstream     _outFile;           // Open stream when redirected to a file by name.
        const DVBCharset* _dvbCharsetIn;      // DVB character set to interpret strings without prefix code.
        const DVBCharset* _dvbCharsetOut;     // Preferred DVB character set to generate strings.
        uint16_t          _casId;             // Preferred CAS id.
        PDS               _defaultPDS;        // Default PDS value if undefined.
        Standards         _cmdStandards;      // Forced standards from the command line.
        Standards         _accStandards;      // Accumulated list of standards in the context.
        UString           _hfDefaultRegion;   // Default region for UHF/VHF band. Empty until used for the first time.
        int               _definedCmdOptions; // Defined command line options.
        const std::map<uint16_t, const UChar*> _predefined_cas;  // Predefined CAS names, index by CAS id (first in range).

        // List of command line options to define and analyze.
        enum CmdOptions {
            CMD_DVB_CHARSET = 0x0001,
            CMD_HF_REGION   = 0x0002,
            CMD_STANDARDS   = 0x0004,
            CMD_PDS         = 0x0008,
            CMD_CAS         = 0x0010,
        };

        // Define several classes of command line options in an Args.
        void defineOptions(Args& args, int cmdOptionsMask);
    };
}
