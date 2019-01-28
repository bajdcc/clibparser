#include "/include/io"
#include "/include/fs"
#include "/include/memory"
int main(int argc, char **argv) {
    if (argc > 1) {
        switch (mkdir(argv[1])) {
            case 0:
                // put_string("[INFO] Success.\n");
                break;
            case -1:
                put_string("[ERROR] Directory already exists.\n");
                break;
            case -2:
                put_string("[ERROR] Path is invalid.\n");
                break;
        }
    }
    return 0;
}