//
// Project: clibparser
// Created by bajdcc
//

// 内存管理
char *malloc(int size) {
    size;
    interrupt 30;
}
int free(int addr) {
    addr;
    interrupt 31;
}
char *memmove(char *dst, char *src, int count) {
    char* tmpdst = dst;
    char* tmpsrc = src;
    if (tmpdst <= tmpsrc || tmpdst >= tmpsrc + count) {
        while(count--) {
            *tmpdst++ = *tmpsrc++;
        }
    } else {
        tmpdst = tmpdst + count - 1;
        tmpsrc = tmpsrc + count - 1;
        while(count--) {
            *tmpdst-- = *tmpsrc--;
        }
    }
    return dst;
}
