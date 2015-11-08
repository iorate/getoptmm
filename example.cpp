#include "getoptmm.hpp"
#include <iterator>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    // http://www.kmonos.net/alang/boost/classes/program_options.html

    using namespace getoptmm;

    bool help = false;
    std::string op;
    int lhs = 100;
    int rhs = 200;

    option opts[] = {
        {{'h'}, {"help"}, no_arg,       store_true(help),        "ヘルプを表示"},
        {{},    {"op"},   required_arg, store(op),        "OP",  "演算の種類(add,sub,mul,div)"},
        {{'L'}, {"lhs"},  required_arg, store(lhs),       "LHS", "左(既定値:100)"},
        {{'R'}, {"rhs"},  required_arg, store(rhs),       "RHS", "右(既定値:200)"}
    };
    parser p(std::begin(opts), std::end(opts), ignore/*ignore non-option arguments*/);
    try {
        p.run(argc, argv);
    } catch (parser::error const &e) {
        std::cerr << e.get_message() << "\n\n";
        std::cout << p.get_help("オプション") << '\n';
        return 1;
    }

    if (help || op.empty()) {
        std::cout << p.get_help("オプション") << '\n';
    } else {
        if (op == "add") std::cout << lhs + rhs << '\n';
        else if (op == "sub") std::cout << lhs - rhs << '\n';
        else if (op == "mul") std::cout << lhs * rhs << '\n';
        else if (op == "div") std::cout << lhs / rhs << '\n';
    }
}
