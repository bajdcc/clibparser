//
// Project: clibparser
// Created by bajdcc
//

#ifndef CLIBPARSER_CVFS_H
#define CLIBPARSER_CVFS_H

#include <vector>
#include <map>
#include <memory>
#include "memory.h"

namespace clib {

    // 权限
    struct vfs_mod {
        char read;
        char write;
        char execute;
    };

    enum vfs_file_t {
        fs_file,
        fs_dir,
    };

    // 结点
    struct vfs_node {
        using ref = std::shared_ptr<vfs_node>;
        using weak_ref = std::shared_ptr<vfs_node>;
        vfs_file_t type;
        uint32_t size;
        vfs_mod mod[3];
        int owner;
        struct {
            time_t create;
            time_t access;
            time_t modify;
        } time;
        std::map<string_t, ref> children;
        weak_ref parent;
    };

    struct vfs_user {
        int id;
        string_t name;
        string_t password;
    };

    class cvfs {
    public:
        cvfs();

        void reset();
        string_t get_user() const;
        string_t get_pwd() const;
        int cd(const string_t &path);
        int mkdir(const string_t &path);

    private:
        vfs_node::ref new_node(vfs_file_t type);
        vfs_node::ref get_node(const string_t &path) const;
        int _mkdir(const string_t &path);

        string_t combine(const string_t &pwd, const string_t &path) const;

        void error(const string_t &);

    private:
        std::vector<vfs_user> account;
        std::shared_ptr<vfs_node> root;
        int current_user;
        string_t pwd;
    };
}

#endif //CLIBPARSER_CVFS_H
