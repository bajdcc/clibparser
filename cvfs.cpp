//
// Project: clibparser
// Created by bajdcc
//

#include <ctime>
#include "cvfs.h"
#include "cexception.h"

namespace clib {

    cvfs::cvfs() {
        reset();
    }

    void cvfs::reset() {
        account.clear();
        account.push_back(vfs_user{0, "root", "root"});
        current_user = 0;
        root = new_node(fs_dir);
    }

    void cvfs::error(const string_t &str) {
        throw cexception(ex_vm, str);
    }

    static void mod_copy(vfs_mod *mod, const char *s) {
        for (int i = 0; i < 9; ++i) {
            ((char *) mod)[i] = *s++;
        }
    }

    vfs_node::ref cvfs::new_node(vfs_file_t type) {
        auto node = std::make_shared<vfs_node>();
        node->type = type;
        node->size = 0;
        node->size = 0;
        if (type == fs_file) {
            mod_copy(node->mod, "rw-rw-rw-");
        } else if (type == fs_dir) {
            mod_copy(node->mod, "rwxrwxr--");
        } else {
            error("invalid mod");
        }
        time_t ctime;
        time(&ctime);
        node->time.create = ctime;
        node->time.access = ctime;
        node->time.modify = ctime;
        node->owner = current_user;
        return node;
    }

    string_t cvfs::user() const {
        return account[current_user].name;
    }
}
