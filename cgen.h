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
        s_sym_id,
    };

    class sym_t {
    public:
        virtual symbol_t get_type() const;
        virtual int size() const;
        virtual string_t to_string() const;
    };

    class type_t : public sym_t {
    public:
        explicit type_t(int ptr = 0);
        symbol_t get_type() const override;
        int ptr;
    };

    class type_base_t : public type_t {
    public:
        explicit type_base_t(lexer_t type, int ptr = 0);
        symbol_t get_type() const override;
        int size() const override;
        string_t to_string() const override;
        lexer_t type;
    };

    class type_typedef_t : public type_t {
    public:
        symbol_t get_type() const override;
        std::weak_ptr<sym_t> sym;
    };

    enum sym_class_t {
        z_undefined,
        z_global_var,
        z_local_var,
        z_end,
    };

    const string_t &sym_class_string(sym_class_t);

    class sym_id_t : public sym_t {
    public:
        explicit sym_id_t(const std::shared_ptr<type_t>& base, const string_t &id);
        symbol_t get_type() const override;
        string_t to_string() const override;
        std::shared_ptr<type_t> base;
        string_t id;
        sym_class_t clazz{z_undefined};
        uint addr{0};
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
        void gen_coll(const std::vector<ast_node *> &nodes, int level, ast_node *node);

        void allocate(sym_id_t &id);

        void error(const string_t &);

    private:
        std::vector<LEX_T(int)> text; // 代码
        std::vector<LEX_T(char)> data; // 数据
        std::vector<std::unordered_map<LEX_T(string), std::shared_ptr<sym_t>>> symbols; // 符号表
        std::vector<std::vector<ast_node *>> ast;
        std::vector<std::vector<std::shared_ptr<sym_t>>> tmp;
    };
}

#endif //CLIBPARSER_CGEN_H
