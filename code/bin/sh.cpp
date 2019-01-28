#include "/include/exec"
#include "/include/io"
#include "/include/fs"
#include "/include/memory"
#include "/include/proc"
#include "/include/string"
#include "/include/sys"
int exec_single(char *text, int *total) {
    while (*text == ' ')
        text++;
    if (strncmp(text, "/sys/", 5) == 0) {
        return -3;
    }
    (*total)++;
    return exec_sleep(text);
}
int exec_start(char *text, int *total) {
    char *c = strchr(text, '|');
    if (c == (char *) 0) {
        return exec_single(text, total);
    } else {
        *c++ = '\0';
        if (*c == '\0')
            return exec_single(text, total);
        int right = exec_start(c, total);
        if (right < 0)
            return right;
        int left = exec_single(text, total);
        exec_connect(left, right);
        exec_wakeup(right);
        return left;
    }
}
int main(int argc, char **argv) {
    int i, j, total, state = 1, direct_input = input_state();
    char *text = malloc(100);
    char *_whoami = malloc(100);
    char *_hostname = malloc(100);
    char *_pwd = malloc(100);
    while (state) {
        total = 0;
        if (direct_input) {
            put_string("[");
            whoami(_whoami);
            put_string(_whoami);
            put_string("@");
            hostname(_hostname);
            put_string(_hostname);
            put_string(" ");
            pwd(_pwd);
            put_string(_pwd);
            put_string("]# ");
        }
        state = input(text, 100);
        if (strlen(text) == 0)
            continue;
        if (strcmp(text, "exit") == 0)
            break;
        int pid = exec_start(text, &total);
        if (pid >= 0) {
            exec_wakeup(pid);
            for (j = 0; j < total; ++j) {
                wait();
            }
        } else {
            exec_kill_children();
            switch (pid) {
                case -1:
                    put_string("[ERROR] File not exists.\n");
                    break;
                case -2:
                    put_string("[ERROR] Compile failed.\n");
                    break;
                case -3:
                    put_string("[ERROR] Cannot execute system programs!\n");
                    break;
            }
        }
        if (!direct_input)
            break;
    }
    free((int) text);
    free((int) _whoami);
    free((int) _hostname);
    free((int) _pwd);
    return 0;
}