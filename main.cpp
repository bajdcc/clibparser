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
int *a, *_a, _b = 1;
char *_c = "Hello world!";
double *b;
int *main1(int c, float *d, sx x, char y) {
    unsigned int *a, *b;
    float d, *e, f;
    sx *s1;
    //sy *s2;
}
int main2() {
    int a, b, c;
    a + b + 1 - 2;
    a = b = c;
    //a.b----[1](1,2);
}
int ta, test(unsigned int a, double b, char c), tb(long d), tc, td(int e);
char *aa = "bajdcc";
int bb = 2;
int dd = bb;
int main(){
    char *ee = "hello";
    char *ff = aa;
    char *gg = ff;
    int cc = 1;
    aa;
    bb;
    bb + 1;
    dd;
    cc + 1;
    int x = 1, y = 2, z = 3;
    x += y *= z;
    z = x < y;
    z = 1 * 2 / 3 + 4;
    ++z + z;
    z++ + z;
    &z;
    *ee;
    *(ee + 2);
    *ee = '9';
    ee[2];
    ee[2] = '8';
    z = 2;
    --z+++--z++;
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