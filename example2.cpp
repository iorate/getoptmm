#include "getoptmm.hpp"
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

template <class T>
std::ostream &operator<<(std::ostream &os, std::vector<T> const &v)
{
    os << '[';
    auto first = true;
    for (auto const &e : v) {
        if (first) { first = false; }
        else { os << ','; }
        os << e;
    }
    os << ']';
    return os;
}

int main(int argc, char *argv[])
{
    // https://hackage.haskell.org/package/base-4.8.1.0/docs/System-Console-GetOpt.html

    using namespace getoptmm;

    bool verbose = false;
    bool version = false;
    std::string output;
    std::string input;
    std::vector<std::string> libdirs;
    std::vector<std::string> files;

    option opts[] = {
        {{'v'},     {"verbose"}, no_arg,       store_true(verbose),                "chatty output on stderr"},
        {{'V','?'}, {"version"}, no_arg,       store_true(version),                "show version number"},
        {{'o'},     {"output"},  optional_arg, store_or(output, "stdout"), "FILE", "output FILE"},
        {{'c'},     {},          optional_arg, store_or(input, "stdin"),   "FILE", "input FILE"},
        {{'L'},     {"libdir"},  required_arg, push_back(libdirs),         "DIR",  "library directory"}
    };

    auto nonopts = push_back(files);

    parser p(std::begin(opts), std::end(opts), nonopts);

    try {
        p.run(argc, argv);

        std::cout << std::boolalpha <<
            "verbose=" << verbose << '\n' <<
            "version=" << version << '\n' <<
            "output="  << output  << '\n' <<
            "input="   << input   << '\n' <<
            "libdirs=" << libdirs << '\n' <<
            "files="   << files   << '\n';

    } catch (parser::error const &e) {
        std::cerr << e.get_message() << "\n\n";
        std::cout << p.get_help("Usage: ic [OPTION...] files...") << '\n';
    }
}
