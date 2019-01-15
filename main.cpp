#include <iostream>
#include "cexception.h"
#include "cparser.h"
#include "cgen.h"

int main() {
    using namespace clib;
    try {
        cparser p;
        cgen gen;
        auto root = p.parse(R"(
struct sx {
    int *a;
    double b;
};
int *a, *_a;
double *b;
int *main(int c, float *d, sx x, char y) {
    unsigned int *a, *b;
    float d, *e, f;
    sx *s1;
    sy *s2;
}
int *c;
int ta, test(unsigned int a, double b, char c), tb(long d), tc, td(int e);
)", &gen);
        cast::print(root, 0, std::cout);
        gen.gen(root);
    } catch (const cexception &e) {
        std::cout << "RUNTIME ERROR: " << e.msg << std::endl;
    }
    return 0;
}