#include "/include/io"
#include "/include/fs"
#include "/include/memory"
#include "/include/shell"
#include "/include/string"
int main(int argc, char **argv) {
    char *s = malloc(1024);
    pwd(s);
    if (argc == 1) { // ls
        char *cmd = malloc(1024);
        strcpy(cmd, "cat ");
        strcat(cmd, s);
        strcat(cmd, ":ls");
        shell(cmd);
        free((int) cmd);
    } else if (argc == 2 && strcmp(argv[1], "-l") == 0) { // ls -l
        set_fg(240, 0, 0);
        put_string("[Error] Not implemented.\n");
        restore_fg();
    } else if (argc == 2) { // ls -l
        char *cmd = malloc(1024);
        strcpy(cmd, "cat ");
        strcat(cmd, argv[1]);
        strcat(cmd, ":ls");
        shell(cmd);
        free((int) cmd);
    } else {
        set_fg(240, 0, 0);
        put_string("[Error] Invalid argument.\n");
        restore_fg();
    }
    free((int) s);
    return 0;
}