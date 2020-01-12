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
//
//  List DVB devices characteristics.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTuner.h"
#include "tsTunerArgs.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;
TS_MAIN(MainCode);

#if defined(TS_WINDOWS)
    #include "tsDirectShowTest.h"
#endif


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

class Options: public ts::Args
{
    TS_NOBUILD_NOCOPY(Options);
public:
    Options(int argc, char *argv[]);
    virtual ~Options();

#if defined(TS_WINDOWS)
    ts::DirectShowTest::TestType test_type;  // DirectShow test (Windows only).
#endif

    ts::DuckContext duck;
    ts::TunerArgs   tuner_args;  // Name of device to list (unspecified means all).
};

// Destructor.
Options::~Options() {}

// Constructor.
Options::Options(int argc, char *argv[]) :
    ts::Args(u"List DVB tuner devices", u"[options]"),
#if defined(TS_WINDOWS)
    test_type(ts::DirectShowTest::NONE),
#endif
    duck(this),
    tuner_args(true, true)
{
    // Common tuner options.
    tuner_args.defineArgs(*this);

#if defined(TS_WINDOWS)

    option(u"enumerate-devices", 'e');
    help(u"enumerate-devices", u"Legacy option, equivalent to --test enumerate-devices.");

    option(u"list-devices", 'l');
    help(u"list-devices", u"Get a list of all tuner and receiver devices, equivalent to --test list-devices.");

    option(u"test", 't', ts::DirectShowTest::TestNames);
    help(u"test", u"name",
         u"Run a specific DirectShow test. Very verbose output, for debug only. "
         u"The default is none.");

#endif

    // Analyze command line options.
    analyze(argc, argv);
    tuner_args.loadArgs(duck, *this);

#if defined(TS_WINDOWS)
    // Test options on Windows. The legacy option "--enumerate-devices" means "--test enumerate-devices".
    if (present(u"list-devices")) {
        test_type = ts::DirectShowTest::LIST_DEVICES;
    }
    else if (present(u"enumerate-devices")) {
        test_type = ts::DirectShowTest::ENUMERATE_DEVICES;
    }
    else {
        test_type = enumValue(u"test", ts::DirectShowTest::NONE);
    }
#endif

    exitOnError();
}


//----------------------------------------------------------------------------
//  This routine lists one tuner device.
//  If tuner_index >= 0, print it (Windows only).
//----------------------------------------------------------------------------

namespace {
    void ListTuner(ts::DuckContext& duck, ts::Tuner& tuner, int tuner_index, Options& opt)
    {
        // If not opened, nothing to display.
        if (!tuner.isOpen()) {
            return;
        }

        // Display name. On Windows, since names are weird, always display
        // the adapter number and use quotes around tuner name.
#if defined(TS_WINDOWS)
        if (tuner_index >= 0) {
            std::cout << tuner_index << ": ";
        }
        std::cout << '"';
#endif
        std::cout << tuner.deviceName();
#if defined(TS_WINDOWS)
        std::cout << '"';
#endif

        // Display tuner information.
        const ts::UString info(tuner.deviceInfo());
        std::cout << " (";
        if (!info.empty()) {
            std::cout << info << ", ";
        }
        std::cout << tuner.deliverySystems().toString() << ")" << std::endl;

        // Display verbose information
        if (opt.verbose()) {
            std::cout << std::endl;
            tuner.displayStatus(std::cout, u"  ", opt);
            std::cout << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);

#if defined(TS_WINDOWS)
    // Specific DirectShow tests on Windows.
    if (opt.test_type != ts::DirectShowTest::NONE) {
        ts::DirectShowTest ds(std::cout, opt);
        ds.runTest(opt.test_type);
        return EXIT_SUCCESS;
    }
#endif

    // List DVB tuner devices
    if (!opt.tuner_args.device_name.empty()) {
        // One device name specified.
        ts::Tuner tuner(opt.duck);
        if (opt.tuner_args.configureTuner(tuner, opt)) {
            ListTuner(opt.duck, tuner, -1, opt);
        }
    }
    else {
        // List all tuners.
        ts::TunerPtrVector tuners;
        if (!ts::Tuner::GetAllTuners(opt.duck, tuners, opt)) {
            return EXIT_FAILURE;
        }
        else if (tuners.empty()) {
            opt.error(u"no DVB device found");
        }
        else {
            if (opt.verbose()) {
                std::cout << std::endl;
            }
            for (size_t i = 0; i < tuners.size(); ++i) {
                ListTuner(opt.duck, *tuners[i], int(i), opt);
            }
        }
    }

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
