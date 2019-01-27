//
// Project: clibparser
// Created by bajdcc
//

// IO输入与输出

// 输出部分
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
int put_hex(int number) {
    number;
    interrupt 2;
}

// 输入部分
int input_char() {
    interrupt 11;
}
int input_lock() {
    interrupt 10;
}
int input_state() {
    interrupt 13;
}
int input(char *text, int len) {
    int i, c;
    int state = input_lock();
    for (i = 0; i < len && ((c = input_char()) != -1); ++i)
        text[i] = c;
    interrupt 12;
    text[i++] = '\0';
    return state;
}

// 界面部分
int resize(int rows, int cols) {
    (rows << 16) | cols;
    interrupt 20;
}
