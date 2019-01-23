enum INTR_TABLE {
    INTR_PUT_CHAR = 0,
    INTR_PUT_NUMBER = 1,
    INTR_PUT_EXEC_CHAR = 50,
    INTR_PUT_EXEC_FILE = 51,
    INTR_PUT_EXEC_WAIT = 52,
    INTR_PUT_FORK = 55,
    INTR_SLEEP_RECORD = 100,
    INTR_SLEEP_REACH = 101,
};
// SYSTEM CALL
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
int exec_char(char c) {
    c;
    interrupt 50;
}
int exec(char *path) {
    while (exec_char(*path++));
    interrupt 51;
}
int wait() {
    interrupt 52;
}
int sleep(int ms) {
    ms;
    interrupt 100;
    interrupt 101;
}
// WELCOME
int welcome() {
    put_string(" ________  ________        ___  ________  ________  ________     \n");
    put_string("|\\   __  \\|\\   __  \\      |\\  \\|\\   ___ \\|\\   ____\\|\\   ____\\    \n");
    put_string("\\ \\  \\|\\ /\\ \\  \\|\\  \\     \\ \\  \\ \\  \\_|\\ \\ \\  \\___|\\ \\  \\___|    \n");
    put_string(" \\ \\   __  \\ \\   __  \\  ___\\ \\  \\ \\  \\ \\\\ \\ \\  \\    \\ \\  \\       \n");
    put_string("  \\ \\  \\|\\  \\ \\  \\ \\  \\|\\  \\\\_\\  \\ \\  \\_\\\\ \\ \\  \\____\\ \\  \\_____\n");
    put_string("   \\ \\_______\\ \\__\\ \\__\\ \\________\\ \\_______\\ \\_______\\ \\_______\\\n");
    put_string("    \\|_______|\\|__|\\|__|\\|________|\\|_______|\\|_______|\\|_______|\n");
    put_string("\n\n");
    put_string("Welcome to @clibos system by bajdcc!");
    put_string("\n\n");
}
int main(int argc, char **argv) {
    welcome();
    exec("/usr/test_rec");  wait();
    exec("/usr/test_fork"); wait();
    return 0;
}