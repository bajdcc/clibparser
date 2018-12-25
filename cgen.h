//
// Project: clibparser
// Created by bajdcc
//

#ifndef CLIBPARSER_CGEN_H
#define CLIBPARSER_CGEN_H

#include <vector>
#include "cast.h"

namespace clib {

    struct sym_t {
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
        std::vector<std::unordered_map<LEX_T(string), sym_t>> symbols; // 符号表
    };
}

#endif //CLIBPARSER_CGEN_H
