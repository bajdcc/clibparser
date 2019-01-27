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
