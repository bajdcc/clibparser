//
// Project: clibparser
// Created by bajdcc
//

#include "/include/string"

// 启动进程
int exec_char(char c) {
    c;
    interrupt 50;
}
int exec(char *path) {
    while (exec_char(*path++));
    interrupt 51;
}
int exec_sleep(char *path) {
    while (exec_char(*path++));
    interrupt 53;
}
int exec_wakeup(int pid) {
    pid;
    interrupt 54;
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
int exec_connect(int left, int right) {
    (left << 16) | right;
    interrupt 56;
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
int exec_kill_children() {
    interrupt 57;
}