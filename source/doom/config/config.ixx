module;

export module config;

import std;

import nstd;
import strings;
import numbers;

export class CommandLine
{
public:
    using iterator = vector<string_view>::iterator;

    static void Initialize(string_view source);

    static string_view Get() { return commandLine; }
    static const vector<string_view>& GetArgs() { return args; }
    static int32 GetArgCount() { return args.size(); }

    static bool HasArg(string_view name);
    static bool HasArg(const std::function<bool(string_view)>& condition);

    static bool GetValueList(string_view name, vector<string_view>& out)
    {
        out.clear();

        auto arg = args.find(name);
        if (arg == args.end())
            return false;

        auto start = arg + 1;
        while (arg != args.end() && arg->front() != '-') ++ arg;
        out.insert(out.end(), start, arg);

        return true;
    }

    static bool TryGetValues(string_view name, auto&& ...out)
    {
        auto arg = args.find(name);
        if (arg == args.end())
            return false;

        return ConvertPack(++arg, out...);
    }

    template<typename T>
    static T GetValue(string_view name, T def = {})
    {
        auto arg = args.find(name);
        if (arg == args.end() || arg + 1 == args.end())
            return def;

        return convert<T>(*(arg + 1));
    }

private:
    static void Parse(string_view source);
    static void ParseResponseFile(string_view file);

    template<typename T>
    static bool ConvertPack(iterator arg, T& out, auto&&... more)
    {
        if (arg == args.end())
            return false;

        out = nstd::convert<T>(*(arg));
        return ConvertPack(++arg, more...);
    }

    template<typename T>
    static bool ConvertPack(iterator arg, T& out)
    {
        if (arg == args.end())
            return false;

        out = nstd::convert<T>(*(arg));
        return true;
    }

    static string_view commandLine;
    static vector<string_view> args;
    static vector<string_view> fileStack;
    static vector<char*> responseFileContent;
};
