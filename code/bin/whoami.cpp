#include "/include/io"
#include "/include/fs"
#include "/include/memory"
int main(int argc, char **argv) {
    char *s = malloc(1024);
    whoami(s);
    put_string(s);
    put_string("\n");
    free((int) s);
    return 0;
}