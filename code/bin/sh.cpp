#include "/include/exec"
#include "/include/io"
#include "/include/fs"
#include "/include/memory"
#include "/include/proc"
#include "/include/string"
#include "/include/sys"
int process(char *text) {
    char *tmp = malloc(256), c;
    int i = 0, j = 0;
    while (true) {
        c = text[i];
        if (c == '>') {
            i++;
            if (text[i] == '>') { // append
                i++;
                strcpy(tmp + j, "| append ");
                j += 9;
            } else { // truncate
                strcpy(tmp + j, "| write ");
                j += 8;
            }
        }
        tmp[j++] = text[i++];
        if (c == (char) 0)
            break;
    }
    strcpy(text, tmp);
}
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
    char *text = malloc(256);
    char *_whoami = malloc(100);
    char *_hostname = malloc(100);
    char *_pwd = malloc(100);
    while (state) {
        total = 0;
        if (direct_input) {
            set_fg(143, 164, 174);
            put_string("[");
            set_fg(63, 234, 53);
            whoami(_whoami);
            put_string(_whoami);
            put_string("@");
            hostname(_hostname);
            put_string(_hostname);
            put_string(" ");
            set_fg(73, 240, 229);
            pwd(_pwd);
            put_string(_pwd);
            set_fg(143, 164, 174);
            put_string("]# ");
            restore_fg();
        }
        state = input(text, 100);
        if (strlen(text) == 0)
            continue;
        if (strcmp(text, "exit") == 0)
            break;
        process(text);
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
                    set_fg(240, 0, 0);
                    put_string("[ERROR] File not exists.\n");
                    restore_fg();
                    break;
                case -2:
                    set_fg(240, 0, 0);
                    put_string("[ERROR] Compile failed.\n");
                    restore_fg();
                    break;
                case -3:
                    set_fg(240, 0, 0);
                    put_string("[ERROR] Cannot execute system programs!\n");
                    restore_fg();
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