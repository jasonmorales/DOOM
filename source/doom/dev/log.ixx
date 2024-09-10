export module log;

import std;
import nstd;

export namespace logger {

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

Category Default
{
    .name = "Default",
};

void write([[maybe_unused]] const Category& category, [[maybe_unused]] Verbosity verbosity, const auto& ...args)
{
    constexpr auto hasArgs = sizeof...(args) > 0;
    static_assert(hasArgs, "log::write() - Category and Verbosity specified, but no message provided");

    if constexpr (hasArgs)
        ((std::cout << "[" << category.name << "] ") << ... << args) << "\n";
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

void write(const auto& ...args)
{
    (std::cout << ... << args) << "\n";
}

void error(const auto& ...args) { write(Default, Verbosity::Error, args...); }
void warn(const auto& ...args) { write(Default, Verbosity::Warning, args...); }
void info(const auto& ...args) { write(Default, Verbosity::Info, args...); }

} // export namespace log
