export module log;

import std;
import nstd;

export namespace log {

void write(const auto& ...args)
{
    (std::cout << ... << args) << "\n";
}

} // export namespace log
