//
// Project: clibparser
// Created by bajdcc
//

// 文件系统

int pwd(char *s) {
    s;
    interrupt 60;
}
int whoami(char *s) {
    s;
    interrupt 61;
}
int cd(char *s) {
    s;
    interrupt 62;
}
int mkdir(char *s) {
    s;
    interrupt 63;
}
int touch(char *s) {
    s;
    interrupt 64;
}
int open(char *s) {
    s;
    interrupt 65;
}
int read(int handle) {
    handle;
    interrupt 66;
}
int close(int handle) {
    handle;
    interrupt 67;
}
