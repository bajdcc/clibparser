#include "/include/io"
#include "/include/fs"
int main(int argc, char **argv) {
    if (argc == 1) { // rm
        set_fg(240, 0, 0);
        put_string("[Error] Need path.\n");
        restore_fg();
    } else if (argc == 2) { // rm XX
        switch (rm(argv[1])) {
            case -1:
                set_fg(240, 0, 0);
                put_string("[ERROR] File not exists.\n");
                restore_fg();
                break;
            case -2:
                set_fg(240, 0, 0);
                put_string("[ERROR] File is locked.\n");
                restore_fg();
                break;
            case -3:
                set_fg(240, 0, 0);
                put_string("[ERROR] Delete failed.\n");
                restore_fg();
                break;
        }
    } else {
        set_fg(240, 0, 0);
        put_string("[Error] Invalid argument.\n");
        restore_fg();
    }
    return 0;
}