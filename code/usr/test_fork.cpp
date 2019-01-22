int put_char(char c) {
    c;
    interrupt 0;
}
int put_string(char *text) {
    while (put_char(*text++));
}
int main(int argc, char **argv) {
    put_string("========== [#2 TEST FORK] ==========\n");
    put_string("========== [#2 TEST FORK] ==========\n");
    return 0;
}