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
int sleep(int ms) {
    ms;
    interrupt 100;
    interrupt 101;
}
int wait(int second) {
    while (second > 0) {
        put_string("Waiting... ");
        put_int(second--);
        put_string("s");
        put_string("\r");
        sleep(1000);
    }
    put_string("\n");
}
int main(int argc, char **argv) {
    int i;
    put_string("========== [#3 TEST INPUT] ==========\n");
    put_string("Command:");
    for (i = 0; i < argc; ++i) {
        put_string(" ");
        put_string(argv[i]);
    }
    put_string("\n");
    put_string("Input: ");
    char text[100];
    input((char *) &text, 100);
    put_string("Output: ");
    put_string((char *) &text);
    put_string("\n");
    put_string("Length: ");
    put_int(strlen((char *) &text));
    put_string("\n");
    wait(1);
    return 0;
}