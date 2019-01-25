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
int main(int argc, char **argv) {
    int i;
    for (i = 1; i < argc; ++i) {
        put_string(argv[i]);
        if (i < argc - 1)
            put_string(" ");
    }
    put_string("\n");
    return 0;
}