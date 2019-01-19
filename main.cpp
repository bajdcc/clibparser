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
int test(int i) {
    return i;
}
int main(int argc, char **argv){
    test(1);
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