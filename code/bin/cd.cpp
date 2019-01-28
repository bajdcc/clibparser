#include "/include/io"
#include "/include/fs"
#include "/include/memory"
int main(int argc, char **argv) {
    if (argc > 1) {
        switch (cd(argv[1])) {
            case -1:
                put_string("[ERROR] Path not exists.\n");
                break;
            case -2:
                put_string("[ERROR] Path is not directory.\n");
                break;
        }
    }
    return 0;
}