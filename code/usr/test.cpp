#include "/include/exec"
#include "/include/io"
#include "/include/proc"
int main(int argc, char **argv) {
    exec("/usr/test_rec");    wait();
    exec("/usr/test_fork");   wait();
    exec("/usr/test_input");  wait();
    exec("/usr/test_resize"); wait();
    exec("/usr/test_malloc"); wait();
    return 0;
}