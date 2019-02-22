//
// Project: clibparser
// Created by bajdcc
//

int atoi32(char* str) {
    int temp = 0;
    char *ptr = str;
    if (*str == '-' || *str == '+') {
        str++;
    }
    while (*str != 0) {
        if ((*str < '0') || (*str > '9')) {
            break;
        }
        temp = temp * 10 + (*str - '0');
        str++;
    }
    if (*ptr == '-') {
        temp = -temp;
    }
    return temp;
}

unsigned int atou32(char* str) {
    unsigned int temp = 0U;
    char *ptr = str;
    if (*str == '-' || *str == '+') {
        str++;
    }
    while (*str != 0) {
        if ((*str < '0') || (*str > '9')) {
            break;
        }
        temp = temp * 10U + (unsigned int)(*str - '0');
        str++;
    }
    if (*ptr == '-') {
        temp = ~temp + 1U;
    }
    return temp;
}
