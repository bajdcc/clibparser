#include <iostream>
#include "cexception.h"
#include "cparser.h"
#include "cgen.h"

int main() {
    using namespace clib;
    try {
        cparser p(R"(
int main() {
    unsigned int *a, *b;
}
)");
        auto root = p.parse();
        cast::print(root, 0, std::cout);
        cgen gen;
        gen.gen(root);
    } catch (const cexception &e) {
        std::cout << "RUNTIME ERROR: " << e.msg << std::endl;
    }
    return 0;
}