#include "/include/io"
#include "/include/fs"
#include "/include/string"
#include "/include/memory"
int write_file(int handle) {
    int c;
    int state = input_lock();
    while ((c = input_char()) != -1) {
        r = write(handle, (char) c);
        if (r < 0) {
            switch (r) {
                case -1:
                    set_fg(240, 0, 0);
                    put_string("[ERROR] File not exists.\n");
                    restore_fg();
                    break;
                case -2:
                    set_fg(240, 0, 0);
                    put_string("[ERROR] Forbidden.\n");
                    restore_fg();
                    break;
                case -3:
                    set_fg(240, 0, 0);
                    put_string("[ERROR] Invalid handle.\n");
                    restore_fg();
                    break;
            }
            break;
        }
    }
    input_unlock();
    close(handle);
}
char *trim(char *text) {
    while (*text == ' ')
        text++;
    return text;
};
int main(int argc, char **argv) {
    if (argc > 1) {
        char *path = malloc(256);
        char *p = path;
        *p = '\0';
        int i;
        for (i = 1; i < argc; ++i) {
            strcat(p, argv[i]);
        }
        char *name = trim(path);
        touch(name);
        int handle = open(name);
        free((int) path);
        switch (handle) {
            default:
                // put_string("[INFO] Success.\n");
                write_file(handle);
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