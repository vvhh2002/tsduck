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

#include "tsTablesLoggerFilter.h"
#include "tsTablesLoggerFilterRepository.h"
#include "tsDuckContext.h"
#include "tsSection.h"
#include "tsArgs.h"
#include "tsPAT.h"
TSDUCK_SOURCE;

// Register this section filter in the reposity.
TS_SECTION_FILTER_REGISTER(ts::TablesLoggerFilter);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TablesLoggerFilter::TablesLoggerFilter() :
    _diversified(false),
    _negate_tid(false),
    _negate_tidext(false),
    _psi_si(false),
    _pids(),
    _tids(),
    _tidexts(),
    _pat()
{
}


//----------------------------------------------------------------------------
// Define section filtering command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesLoggerFilter::defineFilterOptions(Args& args) const
{
    args.option(u"diversified-payload", 'd');
    args.help(u"diversified-payload",
              u"Select only sections with \"diversified\" payload. This means that "
              u"section payloads containing the same byte value (all 0x00 or all 0xFF "
              u"for instance) are ignored. Typically, such sections are stuffing and "
              u"can be ignored that way.");

    args.option(u"negate-pid");
    args.help(u"negate-pid",
              u"Negate the PID filter: specified PID's are excluded. "
              u"Warning: this can be a dangerous option on complete transport "
              u"streams since PID's not containing sections can be accidentally "
              u"selected.");

    args.option(u"negate-tid", 'n');
    args.help(u"negate-tid", u"Negate the TID filter: specified TID's are excluded.");

    args.option(u"negate-tid-ext");
    args.help(u"negate-tid-ext", u"Negate the TID extension filter: specified TID extensions are excluded.");

    args.option(u"pid", 'p', Args::PIDVAL, 0, Args::UNLIMITED_COUNT);
    args.help(u"pid", u"pid1[-pid2]",
              u"PID filter: select packets with this PID value or range of PID values. "
              u"Several -p or --pid options may be specified. "
              u"Without -p or --pid option, all PID's are used (this can be a "
              u"dangerous option on complete transport streams since PID's not "
              u"containing sections can be accidentally selected).");

    args.option(u"psi-si");
    args.help(u"psi-si",
              u"Add all PID's containing PSI/SI tables, ie. PAT, CAT, PMT, NIT, SDT "
              u"and BAT. Note that EIT, TDT and TOT are not included. Use --pid 18 "
              u"to get EIT and --pid 20 to get TDT and TOT.");

    args.option(u"tid", 't', Args::UINT8, 0, Args::UNLIMITED_COUNT);
    args.help(u"tid", u"tid1[-tid2]",
              u"TID filter: select sections with this TID (table id) value or range of TID values. "
              u"Several -t or --tid options may be specified. "
              u"Without -t or --tid option, all tables are saved.");

    args.option(u"tid-ext", 'e', Args::UINT16, 0, Args::UNLIMITED_COUNT);
    args.help(u"tid-ext", u"ext1[-ext2]",
              u"TID extension filter: select sections with this table id "
              u"extension value or range of values (apply to long sections only). "
              u"Several -e or --tid-ext options may be specified. "
              u"Without -e or --tid-ext option, all tables are saved.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TablesLoggerFilter::loadFilterOptions(DuckContext& duck, Args& args, PIDSet& initial_pids)
{
    _diversified = args.present(u"diversified-payload");
    _negate_tid = args.present(u"negate-tid");
    _negate_tidext = args.present(u"negate-tid-ext");
    _psi_si = args.present(u"psi-si");
    args.getIntValues(_pids, u"pid");
    args.getIntValues(_tids, u"tid");
    args.getIntValues(_tidexts, u"tid-ext");

    // If any PID was selected, then --negate-pid means all but them.
    if (args.present(u"negate-pid") && _pids.any()) {
        _pids.flip();
    }

    // With --psi-si, accumulate all PSI/SI PID's/
    // Build the list of PID's to filter (--pid and/or --psi-si).
    if (_psi_si) {
        _pids.set(PID_PAT);
        _pids.set(PID_CAT);
        _pids.set(PID_SDT); // also BAT
        _pids.set(PID_NIT);
    }

    // Inform the tables logger of which PID's we initially need.
    if (_pids.any()) {
        // Some PID's are selected, so we want only them.
        initial_pids = _pids;
    }
    else {
        // We do not specify any PID, this means we want them all.
        initial_pids.set();
    }

    // Clear the current PAT.
    _pat.clear();
    return true;
}


//----------------------------------------------------------------------------
// Check if a specific section must be filtered and displayed.
//----------------------------------------------------------------------------

bool ts::TablesLoggerFilter::filterSection(DuckContext& duck, const Section& section, uint16_t cas, PIDSet& more_pids)
{
    // Accumulate PAT data when --psi-si is specified to detect PMT PID's.
    if (_psi_si && section.tableId() == TID_PAT) {
        // Previous state of the PAT.
        const bool was_valid = _pat.isValid();
        const uint8_t previous_version = _pat.version();
        // Clear previous PAT on new version.
        if (_pat.sectionCount() > 0 && previous_version != section.version()) {
            _pat.clear();
        }
        // Add the current section in the PAT if it was not already there.
        if (_pat.sectionCount() <= section.sectionNumber() || _pat.sectionAt(section.sectionNumber()).isNull()) {
            _pat.addSection(SectionPtr(new Section(section, SHARE)), true, true);
        }
        // If a new PAT is now available, analyze it to grab PSI/SI information.
        if (_pat.isValid() && (!was_valid || _pat.version() != previous_version)) {
            const PAT new_pat(duck, _pat);
            if (new_pat.isValid()) {
                // Check NIT PID, if present.
                if (new_pat.nit_pid != PID_NULL && !_pids.test(new_pat.nit_pid)) {
                    // The NIT PID was not yet known.
                    _pids.set(new_pat.nit_pid);
                    more_pids.set(new_pat.nit_pid);
                }
                // Check all PMT PID's.
                for (auto it = new_pat.pmts.begin(); it != new_pat.pmts.end(); ++it) {
                    const PID pmt_pid = it->second;
                    if (pmt_pid != PID_NULL && !_pids.test(pmt_pid)) {
                        // This PMT PID was not yet known.
                        _pids.set(pmt_pid);
                        more_pids.set(pmt_pid);
                    }
                }
            }
        }
    }

    // Is this a selected TID or TID-ext?
    const bool tid_set = _tids.find(section.tableId()) != _tids.end();
    const bool tidext_set = _tidexts.find(section.tableIdExtension()) != _tidexts.end();

    // Return final verdict. For each criteria (--pid, --tid, etc), either the criteria is
    // not specified or the corresponding value matches.
    return
        // Check PID:
        (_pids.none() || _pids.test(section.sourcePID())) &&
        // Check TID:
        (_tids.empty() || (tid_set && !_negate_tid) || (!tid_set && _negate_tid)) &&
        // Check TIDext:
        (!section.isLongSection() || _tidexts.empty() || (tidext_set && !_negate_tidext) || (!tidext_set && _negate_tidext)) &&
        // Diversified payload ok
        (!_diversified || section.hasDiversifiedPayload());
}
