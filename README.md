# getoptmm
A simple command line parser for C++14

## Usage

Let's write a simple echo program:

    // include the header
    #include "getoptmm.hpp" 

    #include <iostream>
    #include <iterator>
    #include <string>
    #include <vector>

    int main(int argc, char *argv[])
    {
        using namespace getoptmm;

        bool help = false;
        int cnt = 1;
        std::string outp;
        std::vector<std::string> args;

        // define options
        option helpopt(
            {'h'},              // short name(s)
            {"help"},           // long name(s)
            no_arg,             // argument type (no_arg, optional_arg, required_arg)
            assign_true(help),  // option handler (see below)
            /**/                // argument name (if argument type is optional_arg or required_arg)
            "show help message" // description for help message
         );
         option opts[] = {
             helpopt,
             {{'c'}, {"count"},  required_arg, assign(cnt),  "N",    "show output N[=1] time(s)"},
             {{'o'}, {"output"}, required_arg, assign(outp), "FILE", "write output to FILE"}
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

             if (help) {
                 // generate help message
                 std::cout << p.get_help("simple-echo [OPTION...] ARGS...") << '\n';
                 return 0;
             }

             while (cnt-- > 0) {
                 for (auto const &a: args) {
                     std::cout << a << ' ';
                 }
                 std::cout << '\n';
             }
         }
         catch (parser::error const &e) {
             // if a parser fails, an object of parser::error is thrown
             std::cerr << e.get_message() << "\n\n";
             std::cout << p.get_help("simple-echo [OPTION...] ARGS...") << '\n';
         }
    };

Compile and run.

    $ clang++ -std=c++14 main.cpp -o simple-echo

    $ ./simple-echo --help
    simple-echo [OPTION...] ARGS...
    -h      --help        show help message
    -c N    --count=N     show output N[=1] time(s)
    -o FILE --output=FILE write output to FILE

    $ ./simple-echo -c3 hello
    hello
    hello
    hello

    $

The parsing of getoptmm is callback-based.

* A handler for option without argument is called as: `handler();`


    bool version = false;

    option opt = {
        {'v'},
        {"version"},
        no_arg,
        [&]() { version = true; }, // same as assign_true(version)
        "show version number"
    };

* A handler for option with argument or non-option is called as: `handler(str);`


    std::vector<std::wstring> include_paths;

    woption wopt = {
        {L'I'},
        {},
        required_arg,
        [&](std::wstring const &arg) { include_path.push_back(arg); } // same as push_back(include_paths)
        L"PATH",
        L"add PATH to include paths"
    };

Some utilities are provided which can be used as handlers: `assign(val)`, `push_back(val)` etc.

## In more detail

Please see the source.
