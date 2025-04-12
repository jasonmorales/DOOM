export module log;

import std;
import nstd;
import platform.debug;


namespace logger {

export {
enum class Verbosity : uint8
{
    Silent,
    Critical,
    Error,
    Warning,
    Info,
    Verbose,
    VeryVerbose,
};

struct Category
{
    string_view name = "";
};

Category Default("Default");
}

void write_internal(const Category& category, Verbosity verbosity, string_view msg)
{
    (verbosity == Verbosity::Error ? std::cerr : std::cout) << msg;
    platform::debug::write(msg.str());
}

export {

void write(const Category& category, Verbosity verbosity, const auto& ...args)
{
    constexpr auto hasArgs = sizeof...(args) > 0;
    static_assert(hasArgs, "log::write() - Category and Verbosity specified, but no message provided");

    if constexpr (hasArgs)
    {
        std::ostringstream msg;
        (msg << ... << args) << "\n";
        write_internal(category, verbosity, msg.str());
    }
}

void write(Category category, const auto& ...args)
{
    constexpr auto hasArgs = sizeof...(args) > 0;
    static_assert(hasArgs, "log::write() - Category specified, but no message provided");

    if constexpr (hasArgs)
        write(category, Verbosity::Info, args...);
}

void write(Verbosity verbosity, const auto& ...args)
{
    constexpr auto hasArgs = sizeof...(args) > 0;
    static_assert(hasArgs, "log::write() - Verbosity specified, but no message provided");

    if constexpr (hasArgs)
        write(Default, verbosity, args...);
}

void error(const auto& ...args) { write(Default, Verbosity::Error, args...); }
void warn(const auto& ...args) { write(Default, Verbosity::Warning, args...); }
void info(const auto& ...args) { write(Default, Verbosity::Info, args...); }
void write(const auto& ...args) { write(Default, Verbosity::Info, args...); }

}} // export namespace log
