//
// Project: clibparser
// Created by bajdcc
//

#ifndef CLIBPARSER_CGEN_H
#define CLIBPARSER_CGEN_H

#include <vector>
#include <memory>
#include "cast.h"

namespace clib {

    enum symbol_t {
        s_sym,
        s_type,
        s_type_base,
        s_type_typedef,
        s_type_ptr_t,
        s_type_array_t,
    };

    class sym_t {
    public:
    };

    class type_t : public sym_t {
    public:
    };

    class type_base_t : public type_t {
    public:
        explicit type_base_t(keyword_t token);
        keyword_t token;
    };

    class type_typedef_t : public type_t {
    public:
        std::weak_ptr<sym_t> sym;
    };

    class type_ptr_t : public type_t {
    public:
        std::shared_ptr<type_t> base;
        int ptr;
    };

    class type_array_t : public type_t {
        std::shared_ptr<type_t> base;
        int size;
    };

    // 生成虚拟机指令
    class cgen {
    public:
        cgen() = default;
        ~cgen() = default;

        cgen(const cgen &) = delete;
        cgen &operator=(const cgen &) = delete;

        void gen(ast_node *node);
    private:
        void gen_rec(ast_node *node, int level);
        void gen_coll(const std::vector<ast_node *> &nodes, int level, coll_t t);

    private:
        std::vector<LEX_T(int)> text; // 代码
        std::vector<LEX_T(char)> data; // 数据
        std::vector<std::unordered_map<LEX_T(string), std::shared_ptr<sym_t>>> symbols; // 符号表
        std::vector<std::vector<std::shared_ptr<sym_t>>> tmp;
    };
}

#endif //CLIBPARSER_CGEN_H
