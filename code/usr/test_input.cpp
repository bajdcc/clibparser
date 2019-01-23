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
    text[i++] = '\0';
    return i - 1;
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
    return 0;
}