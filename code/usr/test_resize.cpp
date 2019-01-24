int resize(int rows, int cols) {
    (rows << 16) | cols;
    interrupt 20;
}
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
int sleep(int ms) {
    ms;
    interrupt 100;
    interrupt 101;
}
int main(int argc, char **argv) {
    int i;
    put_string("========== [#4 TEST RESIZE] ==========\n");
    put_string("Command:");
    for (i = 0; i < argc; ++i) {
        put_string(" ");
        put_string(argv[i]);
    }
    put_string("\n");
    resize(30, 120);
    sleep(1000);
    resize(20, 20);
    sleep(1000);
    resize(30, 84);
    put_string("========== [#4 TEST RESIZE] ==========\n");
    return 0;
}