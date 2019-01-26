int put_char(char c) {
    c;
    interrupt 0;
}
int put_string(char *text) {
    while (put_char(*text++));
}
int put_int(int number) {
    number;
    interrupt 1;
}
int put_hex(int number) {
    number;
    interrupt 2;
}
int input_char() {
    interrupt 11;
}
int input_lock() {
    interrupt 10;
}
int input_state() {
    interrupt 13;
}
int input(char *text, int len) {
    int i, c;
    int state = input_lock();
    for (i = 0; i < len && ((c = input_char()) != -1); ++i)
        text[i] = c;
    interrupt 12;
    text[i++] = '\0';
    return state;
}
int strlen(char *text) {
    int i = 0;
    while (*text++)
        i++;
    return i;
}
int strcpy(char *dst, char *src) {
    while (*dst++ = *src++);
}
int strncpy(char *dst, char *src, int n) {
    while (n-- > 0 && (*dst++ = *src++));
}
int strcmp(char *a, char *b) {
    int len = 0;
    while(*a && *b && (*a == *b)) {
        a++, b++;
    }
    return *a - *b;
}
int strncmp(char *a, char *b, int n) {
    while(n-- && *a && *b) {
        if (*a != *b) {
            return *a - *b;
        } else {
            a++, b++;
        }
    }
    if (n < 0)
        return 0;
    return *a - *b;
}
char *strchr(char *text, char c) {
    for (; *text; ++text)
        if (*text == c)
            return text;
    return (char *) 0;
}
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
int sleep(int ms) {
    ms;
    interrupt 100;
    interrupt 101;
}
int wait() {
    interrupt 52;
}
char *malloc(int size) {
    size;
    interrupt 30;
}
int free(int addr) {
    addr;
    interrupt 31;
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
int main(int argc, char **argv) {
    int i, j, total, state = 1, direct_input = input_state();
    char *text = malloc(100);
    while (state) {
        total = 0;
        if (direct_input)
            put_string("$ ");
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
    return 0;
}