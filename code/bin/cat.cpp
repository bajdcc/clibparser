#include "/include/io"
#include "/include/fs"
int read_file(int handle) {
    int c;
    while (c = read(handle), c >= 0) {
        if (c < 128)
            put_char((char) c);
    }
    switch (c) {
        case -1:
            // put_string("[INFO] Read to the end.\n");
            put_string("\n");
            break;
        case -2:
            set_fg(240, 0, 0);
            put_string("[ERROR] File already deleted.\n");
            restore_fg();
            break;
        case -3:
            set_fg(240, 0, 0);
            put_string("[ERROR] Invalid handle.\n");
            restore_fg();
            break;
    }
    close(handle);
}
int main(int argc, char **argv) {
    if (argc > 1) {
        int handle = open(argv[1]);
        switch (handle) {
            default:
                // put_string("[INFO] Success.\n");
                read_file(handle);
                break;
            case -1:
                set_fg(240, 0, 0);
                put_string("[ERROR] File not exists.\n");
                restore_fg();
                break;
            case -2:
                set_fg(240, 0, 0);
                put_string("[ERROR] Path is not file.\n");
                restore_fg();
                break;
            case -3:
                set_fg(240, 0, 0);
                put_string("[ERROR] File is locked.\n");
                restore_fg();
                break;
        }
    }
    return 0;
}