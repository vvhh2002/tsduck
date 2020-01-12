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

#include "tsArgs.h"
#include "tsSysUtils.h"
#include "tsVersionInfo.h"
#include "tsOutputPager.h"
#include "tsDuckConfigFile.h"
TSDUCK_SOURCE;

// Unlimited number of occurences
const size_t ts::Args::UNLIMITED_COUNT = std::numeric_limits<size_t>::max();

// Unlimited value.
const int64_t ts::Args::UNLIMITED_VALUE = std::numeric_limits<int64_t>::max();

// List of characters which are allowed thousands separators and decimal points in integer values
const ts::UChar* const ts::Args::THOUSANDS_SEPARATORS = u", ";
const ts::UChar* const ts::Args::DECIMAL_POINTS = u".";

// Enumeration description of HelpFormat.
const ts::Enumeration ts::Args::HelpFormatEnum({
    {u"name",        ts::Args::HELP_NAME},
    {u"description", ts::Args::HELP_DESCRIPTION},
    {u"usage",       ts::Args::HELP_USAGE},
    {u"syntax",      ts::Args::HELP_SYNTAX},
    {u"full",        ts::Args::HELP_FULL},
});


//----------------------------------------------------------------------------
// Constructors for ArgValue
//----------------------------------------------------------------------------

ts::Args::ArgValue::ArgValue() :
    string(),
    int_base(0),
    int_count(0)
{
}


//----------------------------------------------------------------------------
// Constructors for IOption
//----------------------------------------------------------------------------

ts::Args::IOption::IOption(const UChar* name_,
                           UChar        short_name_,
                           ArgType      type_,
                           size_t       min_occur_,
                           size_t       max_occur_,
                           int64_t      min_value_,
                           int64_t      max_value_,
                           size_t       decimals_,
                           uint32_t     flags_) :

    name(name_ == nullptr ? UString() : name_),
    short_name(short_name_),
    type(type_),
    min_occur(min_occur_),
    max_occur(max_occur_),
    min_value(min_value_),
    max_value(max_value_),
    decimals(decimals_),
    flags(flags_),
    enumeration(),
    syntax(),
    help(),
    values(),
    value_count(0)
{
    // Provide default max_occur
    if (max_occur == 0) {
        max_occur = name.empty() ? UNLIMITED_COUNT : 1;
    }
    // Handle invalid values
    if (max_occur < min_occur) {
        throw ArgsError(u"invalid occurences for " + display());
    }
    // Parameters are values by definition
    if (name.empty() && type == NONE) {
        type = STRING;
    }
    // Normalize all integer types to INTEGER
    switch (type) {
        case NONE:
        case STRING:
        case TRISTATE:
            min_value = 0;
            max_value = 0;
            break;
        case INTEGER:
            if (max_value < min_value) {
                throw ArgsError(u"invalid value range for " + display());
            }
            break;
        case UNSIGNED:
            min_value = 0;
            max_value = std::numeric_limits<int64_t>::max();
            type = INTEGER;
            break;
        case POSITIVE:
            min_value = 1;
            max_value = std::numeric_limits<int64_t>::max();
            type = INTEGER;
            break;
        case UINT8:
            min_value = 0;
            max_value = 0xFF;
            type = INTEGER;
            break;
        case UINT16:
            min_value = 0;
            max_value = 0xFFFF;
            type = INTEGER;
            break;
        case UINT32:
            min_value = 0;
            max_value = 0xFFFFFFFF;
            type = INTEGER;
            break;
        case PIDVAL:
            min_value = 0;
            max_value = 0x1FFF;
            type = INTEGER;
            break;
        case INT8:
            min_value = -128;
            max_value = 127;
            type = INTEGER;
            break;
        case INT16:
            min_value = -32768;
            max_value = 32767;
            type = INTEGER;
            break;
        case INT32:
            min_value = -TS_CONST64(0x80000000);
            max_value = 0x7FFFFFFF;
            type = INTEGER;
            break;
        default:
            throw ArgsError(UString::Format(u"invalid option type %d", {type}));
    }
}

ts::Args::IOption::IOption(const UChar*       name_,
                           UChar              short_name_,
                           const Enumeration& enumeration_,
                           size_t             min_occur_,
                           size_t             max_occur_,
                           uint32_t           flags_) :

    name(name_ == nullptr ? UString() : name_),
    short_name(short_name_),
    type(INTEGER),
    min_occur(min_occur_),
    max_occur(max_occur_),
    min_value(std::numeric_limits<int>::min()),
    max_value(std::numeric_limits<int>::max()),
    decimals(0),
    flags(flags_),
    enumeration(enumeration_),
    syntax(),
    help(),
    values(),
    value_count(0)
{
    // Provide default max_occur
    if (max_occur == 0) {
        max_occur = name.empty() ? UNLIMITED_COUNT : 1;
    }
    // Handle invalid values
    if (max_occur < min_occur) {
        throw ArgsError(u"invalid occurences for " + display());
    }
}


//----------------------------------------------------------------------------
// Displayable name for IOption
//----------------------------------------------------------------------------

ts::UString ts::Args::IOption::display() const
{
    UString plural(min_occur > 1 ? u"s" : u"");
    if (name.empty()) {
        return u"parameter" + plural;
    }
    else {
        UString n;
        if (short_name != 0) {
            n = u" (-";
            n += short_name;
            n += u')';
        }
        return u"option" + plural + u" --" + name + n;
    }
}


//----------------------------------------------------------------------------
// Description of the option value.
//----------------------------------------------------------------------------

ts::UString ts::Args::IOption::valueDescription(ValueContext ctx) const
{
    const UString s(syntax.empty() ? u"value" : syntax);

    if (type == NONE || (flags & (IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP)) == (IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP)) {
        // No value or value is optional and shall not be documented.
        return UString();
    }
    else if ((flags & IOPT_OPTVALUE) != 0) {
        return (ctx == LONG ? u"[=" : u"[") + s + u"]";
    }
    else if (ctx == ALONE) {
        return s;
    }
    else {
        return SPACE + s;
    }
}


//----------------------------------------------------------------------------
// When the option has an Enumeration type, get a list of all valid names.
//----------------------------------------------------------------------------

ts::UString ts::Args::IOption::optionNames(const UString& separator) const
{
    return enumeration.nameList(separator, u"\"", u"\"");
}


//----------------------------------------------------------------------------
// Complete option help text.
//----------------------------------------------------------------------------

ts::UString ts::Args::IOption::helpText(size_t line_width) const
{
    IndentationContext indent_desc = TITLE;
    UString text;

    // Add option / parameter name.
    if (name.empty()) {
        // This is the parameters (ie. not options).
        indent_desc = PARAMETER_DESC;
        // Print nothing if parameters are undocumented.
        if (help.empty() && syntax.empty()) {
            return UString();
        }
        // Print generic title instead of option names.
        text += HelpLines(TITLE, max_occur <= 1 ? u"Parameter:" : u"Parameters:", line_width);
        text += LINE_FEED;
    }
    else {
        // This is an option.
        indent_desc = OPTION_DESC;
        if (short_name != 0) {
            text += HelpLines(OPTION_NAME, UString::Format(u"-%c%s", {short_name, valueDescription(IOption::SHORT)}), line_width);
        }
        text += HelpLines(OPTION_NAME, UString::Format(u"--%s%s", {name, valueDescription(IOption::LONG)}), line_width);
    }

    // Add option description.
    if (!help.empty()) {
        text += HelpLines(indent_desc, help, line_width);
    }
    else if (name.empty() && !syntax.empty()) {
        // For parameters (no option name previously displayed), use syntax as fallback for help.
        text += HelpLines(indent_desc, syntax, line_width);
    }

    // Document all possible values for enumeration types.
    if (!enumeration.empty() && (flags & (IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP)) != (IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP)) {
        text += HelpLines(indent_desc, u"Must be one of " + optionNames(u", ") + u".", line_width);
    }

    // Document decimal values (with a decimal point).
    if (decimals > 0) {
        text += HelpLines(indent_desc, UString::Format(u"The value may include up to %d meaningful decimal digits.", {decimals}), line_width);
    }

    return text;
}


//----------------------------------------------------------------------------
// Constructor for Args
//----------------------------------------------------------------------------

ts::Args::Args(const UString& description, const UString& syntax, int flags) :
    _subreport(nullptr),
    _iopts(),
    _description(description),
    _shell(),
    _syntax(syntax),
    _intro(),
    _tail(),
    _app_name(),
    _args(),
    _is_valid(false),
    _flags(flags)
{
    adjustPredefinedOptions();
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

void ts::Args::setDescription(const UString& description)
{
    _description = description;
}

void ts::Args::setSyntax(const UString& syntax)
{
    _syntax = syntax;
}

void ts::Args::setIntro(const UString& intro)
{
    _intro = intro;
}

void ts::Args::setTail(const UString& tail)
{
    _tail = tail;
}

void ts::Args::setFlags(int flags)
{
    _flags = flags;
    adjustPredefinedOptions();
}


//----------------------------------------------------------------------------
// Adjust predefined options based on flags.
//----------------------------------------------------------------------------

void ts::Args::adjustPredefinedOptions()
{
    // Option --help[=value].
    if ((_flags & NO_HELP) != 0) {
        _iopts.erase(u"help");
    }
    else if (_iopts.find(u"help") == _iopts.end()) {
        addOption(IOption(u"help", 0, HelpFormatEnum, 0, 1, IOPT_PREDEFINED | IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP));
        help(u"help", u"Display this help text.");
    }

    // Option --version[=value].
    if ((_flags & NO_VERSION) != 0) {
        _iopts.erase(u"version");
    }
    else if (_iopts.find(u"version") == _iopts.end()) {
        addOption(IOption(u"version", 0,  VersionFormatEnum, 0, 1, IOPT_PREDEFINED | IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP));
        help(u"version", u"Display the TSDuck version number.");
    }

    // Option --verbose.
    if ((_flags & NO_VERBOSE) != 0) {
        _iopts.erase(u"verbose");
    }
    else if (_iopts.find(u"verbose") == _iopts.end()) {
        addOption(IOption(u"verbose", 'v', NONE, 0, 1, 0, 0, 0, IOPT_PREDEFINED));
        help(u"verbose", u"Produce verbose output.");
    }

    // Option --debug[=value].
    if ((_flags & NO_DEBUG) != 0) {
        _iopts.erase(u"debug");
    }
    else if (_iopts.find(u"debug") == _iopts.end()) {
        addOption(IOption(u"debug", 'd', POSITIVE, 0, 1, 0, 0, 0, IOPT_PREDEFINED | IOPT_OPTVALUE));
        help(u"debug", u"level", u"Produce debug traces. The default level is 1. Higher levels produce more messages.");
    }
}


//----------------------------------------------------------------------------
// Format help lines from a long text.
//----------------------------------------------------------------------------

ts::UString ts::Args::HelpLines(IndentationContext level, const UString& text, size_t line_width)
{
    // Actual indentation width.
    size_t indent = 0;
    if (level == PARAMETER_DESC || level == OPTION_NAME) {
        indent = 2;
    }
    else if (level == OPTION_DESC) {
        indent = 6;
    }

    // Format the line.
    const UString margin(indent, SPACE);
    return (margin + text.toTrimmed()).toSplitLines(line_width, u".,;:", margin) + u"\n";
}


//----------------------------------------------------------------------------
// Format the help options of the command.
//----------------------------------------------------------------------------

ts::UString ts::Args::formatHelpOptions(size_t line_width) const
{
    UString text;

    // Set introduction text.
    if (!_intro.empty()) {
        text = HelpLines(TITLE, _intro, line_width);
    }

    // Build a descriptive string from individual options.
    bool titleDone = false;
    for (auto it = _iopts.begin(); it != _iopts.end(); ++it) {
        const IOption& opt(it->second);
        if (!text.empty()) {
            text += LINE_FEED;
        }
        // When this is an option, add 'Options:' the first time.
        if (!titleDone && !opt.name.empty()) {
            titleDone = true;
            text += HelpLines(TITLE, u"Options:", line_width);
            text += LINE_FEED;
        }
        text += opt.helpText(line_width);
    }

    // Set final text.
    if (!_tail.empty()) {
        text += LINE_FEED;
        text.append(HelpLines(TITLE, _tail, line_width));
    }
    return text;
}


//----------------------------------------------------------------------------
// Add a new option.
//----------------------------------------------------------------------------

void ts::Args::addOption(const IOption& opt)
{
    // Erase previous version, if any.
    _iopts.erase(opt.name);

    // If the new option has a short name, erase previous options with same short name.
    if (opt.short_name != 0) {
        for (IOptionMap::iterator it = _iopts.begin(); it != _iopts.end(); ++it) {
            if (it->second.short_name == opt.short_name) {
                it->second.short_name = 0;
                break; // there was at most one
            }
        }
    }

    // Finally add the new option.
    _iopts.insert(std::make_pair(opt.name, opt));
}


//----------------------------------------------------------------------------
// Add an option definition
//----------------------------------------------------------------------------

ts::Args& ts::Args::option(const UChar* name,
                           UChar        short_name,
                           ArgType      type,
                           size_t       min_occur,
                           size_t       max_occur,
                           int64_t      min_value,
                           int64_t      max_value,
                           bool         optional,
                           size_t       decimals)
{
    addOption(IOption(name, short_name, type, min_occur, max_occur, min_value, max_value, decimals, optional ? uint32_t(IOPT_OPTVALUE) : 0));
    return *this;
}

ts::Args& ts::Args::option(const UChar*       name,
                           UChar              short_name,
                           const Enumeration& enumeration,
                           size_t             min_occur,
                           size_t             max_occur,
                           bool               optional)
{
    addOption(IOption(name, short_name, enumeration, min_occur, max_occur, optional ? uint32_t(IOPT_OPTVALUE) : 0));
    return *this;
}


//----------------------------------------------------------------------------
// Add the help text of an exiting option.
//----------------------------------------------------------------------------

ts::Args& ts::Args::help(const UChar* name, const UString& text)
{
    return help(name, UString(), text);
}

ts::Args& ts::Args::help(const UChar* name, const UString& syntax, const UString& text)
{
    IOption& opt(getIOption(name));
    opt.syntax = syntax;
    opt.help = text;
    return *this;
}


//----------------------------------------------------------------------------
// When an option has an Enumeration type, get a list of all valid names.
//----------------------------------------------------------------------------

ts::UString ts::Args::optionNames(const ts::UChar* name, const ts::UString& separator) const
{
    const IOption& opt(getIOption(name));
    return opt.optionNames(separator);
}


//----------------------------------------------------------------------------
// Copy all option definitions from another Args object. Return this object.
// If override is true, override duplicated options.
//----------------------------------------------------------------------------

ts::Args& ts::Args::copyOptions(const Args& other, const bool replace)
{
    for (IOptionMap::const_iterator it = other._iopts.begin(); it != other._iopts.end(); ++it) {
        if ((it->second.flags & IOPT_PREDEFINED) == 0 && (replace || _iopts.find(it->second.name) == _iopts.end())) {
            addOption(it->second);
        }
    }
    return *this;
}


//----------------------------------------------------------------------------
// Redirect report logging. Redirection cancelled if zero.
//----------------------------------------------------------------------------

ts::Report* ts::Args::redirectReport(Report* rep)
{
    Report* previous = _subreport;
    _subreport = rep;
    if (rep != nullptr && rep->maxSeverity() > this->maxSeverity()) {
        this->setMaxSeverity(rep->maxSeverity());
    }
    return previous;
}


//----------------------------------------------------------------------------
// Adjust debug level, always increase verbosity, never decrease.
//----------------------------------------------------------------------------

void ts::Args::raiseMaxSeverity(int level)
{
    // Propagate to superclass (for this object).
    Report::raiseMaxSeverity(level);

    // Propagate to redirected report, if one is set.
    if (_subreport != nullptr) {
        _subreport->raiseMaxSeverity(level);
    }
}


//----------------------------------------------------------------------------
// Display an error message, as if it was produced during command line analysis.
//----------------------------------------------------------------------------

void ts::Args::writeLog(int severity, const UString& message)
{
    // Process error message if flag NO_ERROR_DISPLAY it not set.
    if ((_flags & NO_ERROR_DISPLAY) == 0) {
        if (_subreport != nullptr) {
            _subreport->log(severity, message);
        }
        else {
            if (severity < Severity::Info) {
                std::cerr << _app_name << ": ";
            }
            else if (severity > Severity::Verbose) {
                std::cerr << _app_name << ": " << Severity::Header(severity);
            }
            std::cerr << message << std::endl;
        }
    }

    // Mark this instance as error if severity <= Severity::Error.
    _is_valid = _is_valid && severity > Severity::Error;

    // Immediately abort application is severity == Severity::Fatal.
    if (severity == Severity::Fatal) {
        ::exit(EXIT_FAILURE);
    }
}


//----------------------------------------------------------------------------
// Exit application when errors were reported.
//----------------------------------------------------------------------------

void ts::Args::exitOnError(bool force)
{
    if (!_is_valid && (force || (_flags & NO_EXIT_ON_ERROR) == 0)) {
        ::exit(EXIT_FAILURE);
    }
}


//----------------------------------------------------------------------------
// Locate an option description. Return 0 if not found
//----------------------------------------------------------------------------

ts::Args::IOption* ts::Args::search(UChar c)
{
    for (IOptionMap::iterator it = _iopts.begin(); it != _iopts.end(); ++it) {
        if (it->second.short_name == c) {
            return &it->second;
        }
    }
    error(UString::Format(u"unknown option -%c", {c}));
    return nullptr;
}


//----------------------------------------------------------------------------
// Locate an option description. Return 0 if not found
//----------------------------------------------------------------------------

ts::Args::IOption* ts::Args::search(const UString& name)
{
    IOption* previous = nullptr;

    for (IOptionMap::iterator it = _iopts.begin(); it != _iopts.end(); ++it) {
        if (it->second.name == name) {
            // found an exact match
            return &it->second;
        }
        else if (!name.empty() && it->second.name.find(name) == 0) {
            // found an abbreviated version
            if (previous == nullptr) {
                // remember this one and continue searching
                previous = &it->second;
            }
            else {
                // another one already found, ambiguous option
                error(u"ambiguous option --" + name + u" (--" + previous->name + u", --" + it->second.name + u")");
                return nullptr;
            }
        }
    }

    if (previous != nullptr) {
        // exactly one abbreviation was found
        return previous;
    }
    else if (name.empty()) {
        error(u"no parameter allowed, use options only");
        return nullptr;
    }
    else {
        error(u"unknown option --" + name);
        return nullptr;
    }
}


//----------------------------------------------------------------------------
// Locate an IOption based on its complete long name.
// Throw ArgsError if option does not exist (application internal error)
//----------------------------------------------------------------------------

ts::Args::IOption& ts::Args::getIOption(const UChar* name)
{
    const UString name1(name == nullptr ? u"" : name);
    IOptionMap::iterator it = _iopts.find(name1);
    if (it != _iopts.end()) {
        return it->second;
    }
    else {
        throw ArgsError(_app_name + u": application internal error, option --" + name1 + u" undefined");
    }
}

const ts::Args::IOption& ts::Args::getIOption(const UChar* name) const
{
    // The non-const version above does not modify the object,
    // it just return a non-const reference.
    return const_cast<Args*>(this)->getIOption(name);
}


//----------------------------------------------------------------------------
// Check if option is present
//----------------------------------------------------------------------------

bool ts::Args::present(const UChar* name) const
{
    return !getIOption(name).values.empty();
}


//----------------------------------------------------------------------------
// Check the number of occurences of the option.
//----------------------------------------------------------------------------

size_t ts::Args::count(const UChar* name) const
{
    return getIOption(name).value_count;
}


//----------------------------------------------------------------------------
// Get the value of an option. The index designates the occurence of
// the option. If the option is not present, or not with this
// occurence, defValue is returned.
//----------------------------------------------------------------------------

ts::UString ts::Args::value(const UChar* name, const UChar* defValue, size_t index) const
{
    const IOption& opt(getIOption(name));
    if (opt.type == INTEGER) {
        throw ArgsError(_app_name + u": application internal error, option --" + opt.name + u" is integer, cannot be accessed as string");
    }
    return index >= opt.values.size() || !opt.values[index].string.set() ? defValue : opt.values[index].string.value();
}

void ts::Args::getValue(UString& value_, const UChar* name, const UChar* defValue, size_t index) const
{
    value_ = value(name, defValue, index);
}


//----------------------------------------------------------------------------
// Return all occurences of this option in a vector
//----------------------------------------------------------------------------

void ts::Args::getValues(UStringVector& values, const UChar* name) const
{
    const IOption& opt(getIOption(name));

    values.clear();
    values.reserve(opt.values.size());

    for (auto it = opt.values.begin(); it != opt.values.end(); ++it) {
        if (it->string.set()) {
            values.push_back(it->string.value());
        }
    }
}


//----------------------------------------------------------------------------
// Get the value of tristate option
//----------------------------------------------------------------------------

void ts::Args::getTristateValue(Tristate& value, const UChar* name, size_t index) const
{
    const IOption& opt(getIOption(name));

    if (opt.type == INTEGER) {
        throw ArgsError(_app_name + u": application internal error, option --" + opt.name + u" is integer, cannot be accessed as tristate");
    }
    if (index >= opt.values.size()) {
        // Option not present, meaning unspecified.
        value = MAYBE;
    }
    else if (!opt.values[index].string.set()) {
        // Option present without value, meaning true.
        value = TRUE;
    }
    else if (!opt.values[index].string.value().toTristate(value)) {
        // Value present but not a valid tristate value. Should not occur if the
        // option was declared using TRISTATE type. So, this must be some string
        // option and we cannot decide the Tristate value.
        value = MAYBE;
    }
}

ts::Tristate ts::Args::tristateValue(const UChar* name, size_t index) const
{
    Tristate value = MAYBE;
    getTristateValue(value, name, index);
    return value;
}


//----------------------------------------------------------------------------
// Get the full command line from the last command line analysis.
//----------------------------------------------------------------------------

ts::UString ts::Args::commandLine() const
{
    UString line(_app_name.toQuoted());
    if (!_args.empty()) {
        line.append(SPACE);
        line.append(UString::ToQuotedLine(_args));
    }
    return line;
}


//----------------------------------------------------------------------------
// Load arguments and analyze them, overloads.
//----------------------------------------------------------------------------

bool ts::Args::analyze(const UString& command, bool processRedirections)
{
    UString app;
    UStringVector args;
    command.fromQuotedLine(args);
    if (!args.empty()) {
        app = args.front();
        args.erase(args.begin());
    }
    return analyze(app, args, processRedirections);
}

bool ts::Args::analyze(int argc, char* argv[], bool processRedirections)
{
    UString app;
    UStringVector args;
    if (argc > 0) {
        app = BaseName(UString::FromUTF8(argv[0]), TS_EXECUTABLE_SUFFIX);
        UString::Assign(args, argc - 1, argv + 1);
    }
    return analyze(app, args, processRedirections);
}


//----------------------------------------------------------------------------
// Common code: analyze the command line.
//----------------------------------------------------------------------------

bool ts::Args::analyze(const UString& app_name, const UStringVector& arguments, bool processRedirections)
{
    // Save command line and arguments.
    _app_name = app_name;
    _args = arguments;

    // Clear previous values
    for (IOptionMap::iterator it = _iopts.begin(); it != _iopts.end(); ++it) {
        it->second.values.clear();
        it->second.value_count = 0;
   }

    // Process default arguments from configuration file.
    if ((_flags & NO_CONFIG_FILE) == 0) {
        // Prepend and append default options.
        UStringVector pre;
        UStringVector post;
        DuckConfigFile::Instance()->value(u"prepend.options").splitShellStyle(pre);
        DuckConfigFile::Instance()->value(u"append.options").splitShellStyle(post);
        _args.insert(_args.begin(), pre.begin(), pre.end());
        _args.insert(_args.end(), post.begin(), post.end());

        // Default arguments if there is none.
        if (_args.empty()) {
            DuckConfigFile::Instance()->value(u"default.options").splitShellStyle(_args);
        }
    }

    // Process redirections.
    _is_valid = !processRedirections || processArgsRedirection(_args);

    // Process argument list
    size_t next_arg = 0;            // Index of next arg to process
    size_t short_opt_arg = NPOS;    // Index of arg containing short options
    size_t short_opt_index = NPOS;  // Short option index in _args[short_opt_arg]
    bool force_parameters = false;  // Force all items to be parameters

    while (_is_valid && (short_opt_arg != NPOS || next_arg < _args.size())) {

        IOption* opt = nullptr;
        Variable<UString> val;

        // Locate option name and value
        if (short_opt_arg != NPOS) {
            // Analyzing several short options in a string
            opt = search(_args[short_opt_arg][short_opt_index++]);
            if (short_opt_index >= _args[short_opt_arg].length()) {
                // Reached end of short option string
                short_opt_arg = NPOS;
                short_opt_index = NPOS;
            }
        }
        else if (force_parameters || _args[next_arg].size() < 2 || _args[next_arg][0] != u'-') {
            // Arg is a parameter (can be empty or '-' alone).
            if ((opt = search(u"")) == nullptr) {
                ++next_arg;
            }
            force_parameters = (_flags & GATHER_PARAMETERS) != 0;
        }
        else if (_args[next_arg].length() == 1) {
            // Arg is '-', next arg is a parameter, even if it starts with '-'
            ++next_arg;
            if ((opt = search(u"")) == nullptr) {
                ++next_arg;
            }
        }
        else if (_args[next_arg][1] == '-') {
            // Arg starts with '--', this is a long option
            size_t equal = _args[next_arg].find('=');
            if (equal != NPOS) {
                // Value is in the same arg: --option=value
                opt = search(_args[next_arg].substr(2, equal - 2));
                val = _args[next_arg].substr(equal + 1);
            }
            else {
                // Simple form: --option
                opt = search(_args[next_arg].substr(2));
            }
            ++next_arg;
        }
        else {
            // Arg starts with one single '-'
            opt = search(_args[next_arg][1]);
            if (_args[next_arg].length() > 2) {
                // More short options or value in arg
                short_opt_arg = next_arg;
                short_opt_index = 2;
            }
            ++next_arg;
        }

        // If IOption found...
        if (opt != nullptr) {
            // Get the value string from short option, if present
            if (short_opt_arg != NPOS && opt->type != NONE) {
                assert(!val.set());
                // Get the value from the rest of the short option string
                val = _args[short_opt_arg].substr(short_opt_index);
                short_opt_arg = NPOS;
                short_opt_index = NPOS;
            }

            // Check presence of mandatory values in next arg if not already found
            if (!val.set() && opt->type != NONE && (opt->flags & IOPT_OPTVALUE) == 0 && next_arg < _args.size()) {
                val = _args[next_arg++];
            }

            // Validate option value.
            validateParameter(*opt, val);
        }
    }

    // Process --verbose predefined option
    if ((_flags & NO_VERBOSE) == 0 && present(u"verbose") && (search(u"verbose")->flags & IOPT_PREDEFINED) != 0) {
        raiseMaxSeverity(Severity::Verbose);
    }

    // Process --debug predefined option
    if ((_flags & NO_DEBUG) == 0 && present(u"debug") && (search(u"debug")->flags & IOPT_PREDEFINED) != 0) {
        raiseMaxSeverity(intValue(u"debug", Severity::Debug));
    }

    // Process --help predefined option
    if ((_flags & NO_HELP) == 0 && present(u"help") && (search(u"help")->flags & IOPT_PREDEFINED) != 0) {
        processHelp();
        return _is_valid = false;
    }

    // Process --version predefined option
    if ((_flags & NO_VERSION) == 0 && present(u"version") && (search(u"version")->flags & IOPT_PREDEFINED) != 0) {
        processVersion();
        return _is_valid = false;
    }

    // Look for parameters/options number of occurences.
    // Don't do that if command already proven wrong
    if (_is_valid) {
        for (auto it = _iopts.begin(); it != _iopts.end(); ++it) {
            const IOption& op(it->second);
            if (op.value_count < op.min_occur) {
                error(u"missing " + op.display() + (op.min_occur < 2 ? u"" : UString::Format(u", %d required", {op.min_occur})));
            }
            else if (op.value_count > op.max_occur) {
                error(u"too many " + op.display() + (op.max_occur < 2 ? u"" : UString::Format(u", %d maximum", {op.max_occur})));
            }
        }
    }

    // In case of error, exit
    exitOnError();

    return _is_valid;
}


//----------------------------------------------------------------------------
// Validate the content of an option, add the value.
//----------------------------------------------------------------------------

bool ts::Args::validateParameter(IOption& opt, const Variable<UString>& val)
{
    int64_t last = 0;
    size_t point = NPOS;

    // Build the argument value.
    ArgValue arg;
    arg.string = val;

    if (opt.type == NONE) {
        // There should be no value, this is a flag without value.
        if (val.set()) {
            // In the case --option=value
            error(u"no value allowed for %s", {opt.display()});
            return false;
        }
    }
    else if (!val.set()) {
        // No value set, must be an optional value.
        if ((opt.flags & IOPT_OPTVALUE) == 0) {
            error(u"missing value for %s", {opt.display()});
            return false;
        }
    }
    else if (opt.type == TRISTATE) {
        Tristate t;
        if (!val.value().toTristate(t)) {
            error(u"invalid value %s for %s, use one of %s", {val.value(), opt.display(), UString::TristateNamesList()});
            return false;
        }
    }
    else if (opt.type != INTEGER) {
        // These cases must have been previously eliminated.
        assert(opt.type != UNSIGNED);
        assert(opt.type != POSITIVE);
        assert(opt.type != UINT8);
        assert(opt.type != UINT16);
        assert(opt.type != UINT32);
        assert(opt.type != PIDVAL);
        assert(opt.type != INT8);
        assert(opt.type != INT16);
        assert(opt.type != INT32);
    }
    else if (!opt.enumeration.empty()) {
        // Enumeration value expected, get corresponding integer value (not case sensitive)
        int i = opt.enumeration.value(val.value(), false);
        if (i == Enumeration::UNKNOWN) {
            error(u"invalid value %s for %s, use one of %s", {val.value(), opt.display(), optionNames(opt.name.c_str())});
            return false;
        }
        // Replace with actual integer value.
        arg.int_base = i;
        arg.int_count = 1;
    }
    else if (val.value().toInteger(arg.int_base, THOUSANDS_SEPARATORS, opt.decimals, DECIMAL_POINTS)) {
        // Found exactly one integer value.
        arg.int_count = 1;
    }
    else if ((point = val.value().find(u'-')) != NPOS &&
             point + 1 < val.value().size() &&
             val.value().substr(0, point).toInteger(arg.int_base, THOUSANDS_SEPARATORS, opt.decimals, DECIMAL_POINTS) &&
             val.value().substr(point + 1).toInteger(last, THOUSANDS_SEPARATORS, opt.decimals, DECIMAL_POINTS))
    {
        // Found one range of integer values.
        if (last < arg.int_base) {
            error(u"invalid range of integer values \"%s\" for %s", {val.value(), opt.display()});
            return false;
        }
        arg.int_count = size_t(last + 1 - arg.int_base);
    }
    else {
        error(u"invalid integer value %s for %s", {val.value(), opt.display()});
        return false;
    }

    // Check validity of integer values.
    if (opt.type == INTEGER && arg.int_count > 0) {
        if (arg.int_base < opt.min_value) {
            error(u"value for %s must be >= %'d", {opt.display(), opt.min_value});
            return false;
        }
        else if (arg.int_base + int64_t(arg.int_count) - 1 > opt.max_value) {
            error(u"value for %s must be <= %'d", {opt.display(), opt.max_value});
            return false;
        }
    }

    // Push value. For optional parameters without value, an unset variable is pushed.
    opt.values.push_back(arg);

    // Add the number of occurences. Can be more than one in case of integer range.
    opt.value_count += opt.type == INTEGER && arg.int_count > 0 ? arg.int_count : 1;

    return true;
}


//----------------------------------------------------------------------------
// Get a formatted help text.
//----------------------------------------------------------------------------

ts::UString ts::Args::getHelpText(HelpFormat format, size_t line_width) const
{
    switch (format) {
        case HELP_NAME: {
            // Return the application name as set by the application.
            return _app_name;
        }
        case HELP_DESCRIPTION: {
            // Return the descripton string as set by the application.
            return _description;
        }
        case HELP_USAGE: {
            // Return the usage string with application name and syntax.
            if (_shell.empty()) {
                return _app_name + u" " + _syntax;
            }
            else {
                return _shell + u" " + _app_name + u" " + _syntax;
            }
        }
        case HELP_SYNTAX: {
            // Same as usage but on one line.
            UString str(getHelpText(HELP_USAGE, line_width));
            // Replace all backslash-newline by newline.
            str.substitute(u"\\\n", u"\n");
            // Remove all newlines and compact spaces.
            size_t pos = 0;
            while ((pos = str.find('\n')) != NPOS) {
                // Locate the first space in the sequence.
                while (pos > 0 && IsSpace(str[pos - 1])) {
                    pos--;
                }
                // Replace the first space with a true space.
                str[pos] = ' ';
                // Remove all subsequent spaces.
                while (pos < str.length() - 1 && IsSpace(str[pos + 1])) {
                    str.erase(pos + 1, 1);
                }
            }
            return str;
        }
        case HELP_FULL: {
            // Default full complete help text.
            return u"\n" + _description + u"\n\nUsage: " + getHelpText(HELP_USAGE, line_width) + u"\n\n" + formatHelpOptions(line_width);
        }
        default: {
            return UString();
        }
    }
}


//----------------------------------------------------------------------------
// Process --help predefined option.
//----------------------------------------------------------------------------

void ts::Args::processHelp()
{
    // Build the help text. Use full text by default.
    const HelpFormat format = enumValue(u"help", HELP_FULL);
    const UString text(getHelpText(format));

    // Create a pager process if we intend to exit immediately after a full help text.
    OutputPager pager;
    if (format == HELP_FULL && (_flags & NO_EXIT_ON_HELP) == 0 && pager.canPage() && pager.open(true, 0, *this)) {
        pager.write(text, *this);
        pager.write(u"\n", *this);
        pager.close(*this);
    }
    else if ((_flags & HELP_ON_THIS) != 0) {
        info(text);
    }
    else {
        std::cerr << text << std::endl;
    }

    // Exit application, unless specified otherwise.
    if ((_flags & NO_EXIT_ON_HELP) == 0) {
        ::exit(EXIT_SUCCESS);
    }
}


//----------------------------------------------------------------------------
// Process --version predefined option.
//----------------------------------------------------------------------------

void ts::Args::processVersion()
{
    // The meaning of the option value is managed inside GetVersion.
    info(GetVersion(enumValue(u"version", VERSION_LONG), _app_name));

    // Exit application, unless specified otherwise.
    if ((_flags & NO_EXIT_ON_VERSION) == 0) {
        ::exit(EXIT_SUCCESS);
    }
}


//----------------------------------------------------------------------------
// Process argument redirection using @c '\@' on a vector of strings.
//----------------------------------------------------------------------------

bool ts::Args::processArgsRedirection(UStringVector& args)
{
    bool result = true;

    UStringVector::iterator it = args.begin();
    while (it != args.end()) {
        if (it->startWith(u"@@")) {
            // An initial double @ means a single literal @. Remove the first @.
            it->erase(0, 1);
            ++it;
        }
        else if (it->startWith(u"@")) {
            // Replace the line with the content of a file.

            // Get the file name.
            const UString fileName(it->substr(1));

            // Remove the line from the argument array.
            it = args.erase(it);

            // Load the text file.
            UStringVector lines;
            if (UString::Load(lines, fileName)) {
                // Insert the loaded lines. Then, make "it" point to the first inserted element.
                // This means that the loaded content will now be processed, allowing nested '@' directives.
#if defined(TS_CXX11_STRING)
                // Starting with C++11, std::vector::insert() shall return an iterator.
                it = args.insert(it, lines.begin(), lines.end());
#else
                // However, with GCC, there is the same problem as C++11 strings, GCC 4 claims to
                // be C++11 compliant but in fact it is not. So, we have to manually recompute
                // the iterator after insertion for old versions of GCC.
                const size_t index = it - args.begin();
                args.insert(it, lines.begin(), lines.end());
                it = args.begin() + index;
#endif
            }
            else {
                // Error loading file.
                result = false;
                error(u"error reading command line arguments from file \"%s\"", {fileName});
            }
        }
        else {
            // No leading '@', nothing to do
            ++it;
        }
    }

    return result;
}
