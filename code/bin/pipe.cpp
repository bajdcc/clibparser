#include "/include/io"
int pipe() {
    int c;
    interrupt 10;
    while ((c = input_char()) != -1) {
        put_char((char) c);
    }
    interrupt 12;
    return 0;
}
int main(int argc, char **argv) {
    pipe();
    return 0;
}