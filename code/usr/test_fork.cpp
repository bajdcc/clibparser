int fork() {
    interrupt 55;
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
int wait() {
    interrupt 52;
}
int sleep(int ms) {
    ms;
    interrupt 100;
    interrupt 101;
}
int main(int argc, char **argv) {
    int i;
    put_string("========== [#2 TEST FORK] ==========\n");
    put_string("Command:");
    for (i = 0; i < argc; ++i) {
        put_string(" ");
        put_string(argv[i]);
    }
    put_string("\n");
    i = fork();
    if (i == -1) {
        put_string("Child: fork return "); put_int(i); put_string("\n");
    } else {
        wait();
        sleep(500);
        put_string("Parent: fork return "); put_int(i); put_string("\n");
        put_string("========== [#2 TEST FORK] ==========\n");
    }
    return 0;
}