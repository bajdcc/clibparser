#include "/include/io"
int wc(int *lines) {
    int c;
    interrupt 10;
    while ((c = input_char()) != -1) {
        if (((char) c) == '\n')
            (*lines)++;
    }
    interrupt 12;
    return 0;
}
int main(int argc, char **argv) {
    int lines = 0;
    wc(&lines);
    put_int(lines);
    put_string("\n");
    return 0;
}