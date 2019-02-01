#include "/include/shell"
int main(int argc, char **argv) {
    shell("/usr/test_rec");
    shell("/usr/test_fork");
    shell("/usr/test_input");
    shell("/usr/test_resize");
    shell("/usr/test_malloc");
    return 0;
}