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
        pwd = "/";
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
        node->refs = 0;
        node->locked = false;
        return node;
    }

    string_t cvfs::get_user() const {
        return account[current_user].name;
    }

    string_t cvfs::get_pwd() const {
        return pwd;
    }

    bool cvfs::read_vfs(const string_t &path, std::vector<byte> &data) const {
        auto node = get_node(path);
        if (!node)
            return false;
        if (node->type != fs_file)
            return false;
        data.resize(node->data.size());
        std::copy(node->data.begin(), node->data.end(), data.begin());
        return true;
    }

    bool cvfs::write_vfs(const string_t &path, const std::vector<byte> &data) {
        auto node = get_node(path);
        if (!node) {
            touch(path);
            node = get_node(path);
            if (!node)
                return false;
        }
        if (node->type != fs_file)
            return false;
        node->data.resize(data.size());
        std::copy(data.begin(), data.end(), node->data.begin());
        return true;
    }

    string_t get_parent(const string_t &path) {
        assert(path[0] == '/');
        if (path == "/")
            return path;
        auto f = path.find_last_of('/');
        assert(f != string_t::npos);
        if (f == 0)
            return "/";
        return path.substr(0, f);
    }

    void split_path(const string_t &path, std::vector<string_t> &args) {
        std::stringstream ss(path);
        string_t temp;
        while (std::getline(ss, temp, '/')) {
            args.push_back(temp);
        }
    }

    vfs_node::ref cvfs::get_node(const string_t &path) const {
        std::vector<string_t> paths;
        split_path(path, paths);
        auto cur = root;
        for (auto i = 0; i < paths.size(); ++i) {
            auto &p = paths[i];
            if (!p.empty()) {
                auto f = cur->children.find(p);
                if (f != cur->children.end()) {
                    if (i < paths.size() - 1 && f->second->type != fs_dir)
                        return nullptr;
                    cur = f->second;
                } else {
                    return nullptr;
                }
            }
        }
        return cur;
    }

    int cvfs::cd(const string_t &path) {
        auto p = combine(pwd, path);
        auto node = get_node(p);
        if (!node)
            return -1;
        switch (node->type) {
            case fs_file:
                return -2;
            case fs_dir:
                pwd = p;
                break;
        }
        return 0;
    }

    int cvfs::_mkdir(const string_t &path, vfs_node::ref &cur) {
        std::vector<string_t> paths;
        split_path(path, paths);
        cur = root;
        bool update = false;
        for (auto &p : paths) {
            if (!p.empty()) {
                auto f = cur->children.find(p);
                if (f != cur->children.end()) {
                    cur = f->second;
                    if (f->second->type != fs_dir)
                        return -2;
                } else {
                    if (!update)
                        update = true;
                    auto node = new_node(fs_dir);
                    node->parent = cur;
                    cur->children.insert(std::make_pair(p, node));
                    cur = node;
                }
            }
        }
        if (update)
            return 0;
        return -1;
    }

    int cvfs::mkdir(const string_t &path) {
        auto p = combine(pwd, path);
        vfs_node::ref cur;
        return _mkdir(p, cur);
    }

    string_t cvfs::combine(const string_t &pwd, const string_t &path) const {
        if (path.empty())
            return pwd;
        if (path[0] == '/')
            return path;
        auto res = pwd;
        std::vector<string_t> paths;
        split_path(path, paths);
        for (auto &p : paths) {
            if (!p.empty()) {
                if (p == ".")
                    continue;
                else if (p == "..")
                    res = get_parent(res);
                else if (res.back() == '/')
                    res += p;
                else
                    res += "/" + p;
            }
        }
        return res;
    }

    int cvfs::touch(const string_t &path) {
        auto p = combine(pwd, path);
        auto node = get_node(p);
        if (!node) {
            vfs_node::ref cur;
            auto s = _mkdir(p, cur);
            if (s == 0) { // new dir
                cur->type = fs_file;_touch(cur);
                return -1;
            } else { // exists
                _touch(cur);
                return 0;
            }
        }
        switch (node->type) {
            case fs_file:
            case fs_dir:
                _touch(node);
                return 0;
            default:
                return -2;
        }
    }

    void cvfs::_touch(vfs_node::ref &node) {
        time_t ctime;
        time(&ctime);
        node->time.create = ctime;
        node->time.access = ctime;
        node->time.modify = ctime;
    }
}
