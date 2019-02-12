#include "/include/shell"
int main(int argc, char **argv) {
    int i = 1;
    if (argc > 1) {
        if (argv[1][0] >= '1' && argv[1][0] <= '9')
            i = argv[1][0] - '0';
    }
    switch (i) {
        case 1: shell("/usr/test_rec");
        case 2: shell("/usr/test_fork");
        case 3: shell("/usr/test_input");
        case 4: shell("/usr/test_resize");
        case 5: shell("/usr/test_malloc");
        case 6: shell("/usr/test_struct");
        case 7: shell("/usr/test_xtoa");
    }
    return 0;
}