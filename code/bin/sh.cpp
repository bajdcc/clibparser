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
int input_char() {
    interrupt 11;
}
int input(char *text, int len) {
    int i, c;
    interrupt 10;
    for (i = 0; i < len && ((c = input_char()) != -1); ++i)
        text[i] = c;
    interrupt 12;
    text[i++] = '\0';
    return i - 1;
}
int strlen(char *text) {
    int i = 0;
    while (*text++)
        i++;
    return i;
}
int strcmp(char *a, char *b) {
    int len = 0;
    while(*a && *b && (*a == *b)) {
        a++, b++;
    }
    return *a - *b;
}
int strncmp(char *a, char *b, int n) {
    while(n-- && *a && *b && (*a++ == *b++));
    if (n < 0)
        return 0;
    return *a - *b;
}
int exec_char(char c) {
    c;
    interrupt 50;
}
int exec(char *path) {
    while (exec_char(*path++));
    interrupt 51;
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
int main(int argc, char **argv) {
    int i;
    char *text = malloc(100);
    while (true) {
        put_string("$ ");
        input((char *) &text, 100);
        if (strlen((char *) &text) == 0)
            continue;
        if (strcmp((char *) &text, "exit") == 0)
            break;
        if (strncmp((char *) &text, "/sys/", 5) == 0) {
            put_string("[ERROR] Cannot execute system programs!\n");
            continue;
        }
        switch (exec((char *) &text)) {
            case -1:
                put_string("[ERROR] File not exists.\n");
                break;
            case -2:
                put_string("[ERROR] Compile failed.\n");
                break;
            default:
                wait();
                break;
        }
    }
    return 0;
}