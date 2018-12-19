#include <iostream>
#include "cexception.h"
#include "cparser.h"

int main() {
    using namespace clib;
    try {
        cparser p(R"(
int main() {
    int a, b, c;
    float d, e, f;
}
)");
        auto root = p.parse();
        cast::print(root, 0, std::cout);
    } catch (const cexception &e) {
        std::cout << "RUNTIME ERROR: " << e.msg << std::endl;
    }
    return 0;
}