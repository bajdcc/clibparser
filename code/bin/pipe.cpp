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
int input() {
    int c;
    interrupt 10;
    while ((c = input_char()) != -1) {
        put_char((char) c);
    }
    interrupt 12;
    return 0;
}
int main(int argc, char **argv) {
    input();
    return 0;
}