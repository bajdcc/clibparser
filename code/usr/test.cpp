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
int wait() {
    interrupt 52;
}
int exec_char(char c) {
    c;
    interrupt 50;
}
int exec(char *path) {
    while (exec_char(*path++));
    interrupt 51;
}
int main(int argc, char **argv) {
    exec("/usr/test_rec");    wait();
    exec("/usr/test_fork");   wait();
    exec("/usr/test_input");  wait();
    exec("/usr/test_resize"); wait();
    return 0;
}