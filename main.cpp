#include <iostream>
#include "cexception.h"
#include "cparser.h"
#include "cgen.h"

extern int g_argc;
extern char **g_argv;

int main(int argc, char **argv) {
    g_argc = argc;
    g_argv = argv;
    using namespace clib;
    try {
        cparser p;
        cgen gen;
        auto root = p.parse(R"(
int fib(int i) {
    if (i > 2)
        return fib(i - 1) + fib(i - 2);
    else
        return 1;
}
int sum(int i) {
    int s = 0;
    while (i > 0) {
        s += i--;
    }
    return s;
}
int sum2(int n) {
    int i, s;
    for (i = 1; i <= n; ++i) {
        s += i;
    }
    return s;
}
int main(int argc, char **argv){
    fib(10);
    sum(100);
    sum2(100);
}
)", &gen);
        cast::print(root, 0, std::cout);
        gen.gen(root);
        gen.eval();
    } catch (const cexception &e) {
        std::cout << "RUNTIME ERROR: " << e.msg << std::endl;
    }
    return 0;
}