#include "/include/exec"
#include "/include/io"
#include "/include/fs"
#include "/include/memory"
#include "/include/proc"
#include "/include/string"
int main(int argc, char **argv) {
    char *s = malloc(1024);
    pwd(s);
    if (argc == 1) { // ls
        char *cmd = malloc(1024);
        strcpy(cmd, "cat ");
        strcat(cmd, s);
        strcat(cmd, ":ls");
        exec(cmd);
        wait();
        free((int) cmd);
    } else if (argc == 2 && strcmp(argv[1], "-l") == 0) { // ls -l
        put_string("[Error] Not implemented.\n");
    } else if (argc == 2) { // ls -l
        char *cmd = malloc(1024);
        strcpy(cmd, "cat ");
        strcat(cmd, argv[1]);
        strcat(cmd, ":ls");
        exec(cmd);
        wait();
        free((int) cmd);
    } else {
        put_string("[Error] Invalid argument.\n");
    }
    free((int) s);
    return 0;
}