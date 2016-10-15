# getoptmm
A simple command line parser for C++14

## Usage

Let's write a simple echo program:

```cpp
// include the header
#include "getoptmm.hpp" 

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int main(int argc, char *argv[])
{
    using namespace getoptmm;

    bool help = false;
    int count = 1;
    std::string output;
    std::vector<std::string> args;

    // define options
    option helpopt(
        {'h'},              // short name(s)
        {"help"},           // long name(s)
        no_arg,             // argument type (no_arg, optional_arg, required_arg)
        assign_true(help),  // option handler (see below)
        /**/                // argument name (unless argument type is no_arg)
        "show help message" // description for help message
    );
    option opts[] = {
        helpopt,
        {{'c'}, {"count"},  required_arg, assign(count),  "N",    "show output N time(s)"},
        {{'o'}, {"output"}, required_arg, assign(output), "FILE", "write output to FILE"}
    };

    // create a parser
    parser p(
        std::begin(opts), // options
        std::end(opts),
        push_back(args)   // non-option handler (see below)   
    );

    try {
        // parse
        p.run(argc, argv);
    }
    catch (parser::error const &e) {
        // if a parser fails, an object of parser::error is thrown
        std::cerr << e.message() << "\n\n";

        // generate help message
        std::cout << p.usage_info("simple-echo [OPTION...] ARGS...") << '\n';

        return 1;
    }

    if (help) {
        std::cout << p.usage_info("simple-echo [OPTION...] ARGS...") << '\n';
        return 0;
    }

    // implementation
    std::ofstream file;
    if (!output.empty()) {
        file.open(output);
        if (!file) { return 1; }
    }
    auto &out = output.empty() ? std::cout : file;

    while (count-- > 0) {
        for (auto const &a: args) {
            out << a << ' ';
        }
        out << '\n';
    }
};
```

Compile and run.

```shell
$ clang++ -std=c++14 main.cpp -o simple-echo

$ ./simple-echo --help
simple-echo [OPTION...] ARGS...
-h      --help        show help message
-c N    --count=N     show output N time(s)
-o FILE --output=FILE write output to FILE

$ ./simple-echo -c3 hello
hello
hello
hello

$
```

The parsing of *getoptmm* is callback-based.

* A handler for option without argument is called as: `handler();`

```cpp
bool version = false;

option opt = {
    {'v'},
    {"version"},
    no_arg,
    [&]() { version = true; }, // same as assign_true(version)
    "show version number"
};
```

* A handler for option with argument or non-option is called as: `handler(str);`

```cpp
std::vector<std::wstring> paths;

woption wopt = {
    {L'p'},
    {L"path"},
    required_arg,
    [&](auto const &arg) { paths.push_back(arg); }, // same as push_back(paths)
    L"PATH",
    L"add PATH to paths"
};
```

Some utilities are provided which can be used as handlers: `assign(val)`, `push_back(val)` etc.

## In more detail

Please see the source.
