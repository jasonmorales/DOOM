module;

module log;

import std;

void write(const auto& ...args)
{
    (std::cout << ... << args) << "\n";
}
