//
// Project: clibparser
// Created by bajdcc
//

#include "/include/exec"
#include "/include/proc"

int intern_pipe() {
    int c;
    input_lock();
    while ((c = input_char()) != -1) {
        put_char((char) c);
    }
    input_unlock();
    return 0;
}
int shell(char *path) {
    int pid = exec_sleep(path);
    exec_connect(pid, get_pid());
    exec_wakeup(pid);
    intern_pipe();
    wait();
}
