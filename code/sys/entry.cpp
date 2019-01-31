#include "/include/proc"
#include "/include/exec"
#include "/include/io"
// WELCOME
int welcome() {
    set_fg(0, 240, 240);
    set_bg(43, 43, 43);
    put_string(" ________  ________        ___  ________  ________  ________     \n");
    put_string("|\\   __  \\|\\   __  \\      |\\  \\|\\   ___ \\|\\   ____\\|\\   ____\\    \n");
    put_string("\\ \\  \\|\\ /\\ \\  \\|\\  \\     \\ \\  \\ \\  \\_|\\ \\ \\  \\___|\\ \\  \\___|    \n");
    put_string(" \\ \\   __  \\ \\   __  \\  ___\\ \\  \\ \\  \\ \\\\ \\ \\  \\    \\ \\  \\       \n");
    put_string("  \\ \\  \\|\\  \\ \\  \\ \\  \\|\\  \\\\_\\  \\ \\  \\_\\\\ \\ \\  \\____\\ \\  \\_____\n");
    put_string("   \\ \\_______\\ \\__\\ \\__\\ \\________\\ \\_______\\ \\_______\\ \\_______\\\n");
    put_string("    \\|_______|\\|__|\\|__|\\|________|\\|_______|\\|_______|\\|_______|\n");
    restore_fg();
    restore_bg();
    put_string("\n\n");
    set_fg(160, 160, 160);
    put_string("Welcome to @clibos system by bajdcc!\n\n");
    put_string("# Type \"help\" for help.\n\n");
    restore_fg();
}
int main(int argc, char **argv) {
    welcome();
    exec("sh"); wait();
    return 0;
}