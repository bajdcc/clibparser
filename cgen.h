//
// Project: clibparser
// Created by bajdcc
//

#ifndef CLIBPARSER_CGEN_H
#define CLIBPARSER_CGEN_H

#include <vector>
#include <memory>
#include "cast.h"
#include "cparser.h"

namespace clib {

    enum symbol_t {
        s_sym,
        s_type,
        s_type_base,
        s_type_typedef,
        s_id,
        s_struct,
        s_function,
        s_expression,
        s_statement,
    };

    class sym_t {
    public:
        using ref = std::shared_ptr<sym_t>;
        virtual symbol_t get_type() const;
        virtual symbol_t get_base_type() const;
        virtual int size() const;
        virtual string_t get_name() const;
        virtual string_t to_string() const;
        int line{0}, column{0};
    };

    class type_t : public sym_t {
    public:
        using ref = std::shared_ptr<type_t>;
        explicit type_t(int ptr = 0);
        symbol_t get_type() const override;
        symbol_t get_base_type() const override;
        virtual ref clone() const;
        int ptr;
    };

    class type_base_t : public type_t {
    public:
        explicit type_base_t(lexer_t type, int ptr = 0);
        symbol_t get_type() const override;
        int size() const override;
        string_t get_name() const override;
        string_t to_string() const override;
        type_t::ref clone() const override;
        lexer_t type;
    };

    class type_typedef_t : public type_t {
    public:
        explicit type_typedef_t(const sym_t::ref &refer, int ptr = 0);
        symbol_t get_type() const override;
        int size() const override;
        string_t to_string() const override;
        ref clone() const override;
        std::weak_ptr<sym_t> refer;
    };

    enum sym_class_t {
        z_undefined,
        z_global_var,
        z_local_var,
        z_param_var,
        z_struct_var,
        z_function,
        z_end,
    };

    const string_t &sym_class_string(sym_class_t);

    class sym_id_t : public sym_t {
    public:
        using ref = std::shared_ptr<sym_id_t>;
        explicit sym_id_t(const type_t::ref& base, const string_t &id);
        symbol_t get_type() const override;
        symbol_t get_base_type() const override;
        int size() const override;
        string_t get_name() const override;
        string_t to_string() const override;
        type_t::ref base;
        string_t id;
        sym_class_t clazz{z_undefined};
        uint addr{0};
    };

    class sym_struct_t : public sym_t {
    public:
        using ref = std::shared_ptr<sym_id_t>;
        explicit sym_struct_t(const string_t &id);
        symbol_t get_type() const override;
        symbol_t get_base_type() const override;
        int size() const override;
        string_t get_name() const override;
        string_t to_string() const override;
        string_t id;
        int _size{0};
        std::vector<sym_id_t::ref> decls;
    };

    class sym_func_t : public sym_id_t {
    public:
        explicit sym_func_t(const type_t::ref& base, const string_t &id);
        symbol_t get_type() const override;
        symbol_t get_base_type() const override;
        int size() const override;
        string_t to_string() const override;
        std::vector<sym_id_t::ref> params;
    };

    // 生成虚拟机指令
    class cgen : public csemantic {
    public:
        cgen();
        ~cgen() = default;

        cgen(const cgen &) = delete;
        cgen &operator=(const cgen &) = delete;

        backtrace_direction check(pda_edge_t, ast_node *) override;

        void gen(ast_node *node);
        void reset();
    private:
        void gen_rec(ast_node *node, int level);
        void gen_coll(const std::vector<ast_node *> &nodes, int level, ast_node *node);

        void allocate(sym_id_t &id);
        void add_id(const type_base_t::ref &, sym_class_t, ast_node *);

        void error(const string_t &);
        void error(ast_node *, const string_t &, bool info = false);
        void error(sym_t *, const string_t &);

    private:
        std::vector<LEX_T(int)> text; // 代码
        std::vector<LEX_T(char)> data; // 数据
        std::vector<std::unordered_map<LEX_T(string), std::shared_ptr<sym_t>>> symbols; // 符号表
        std::vector<std::vector<ast_node *>> ast;
        std::vector<std::vector<std::shared_ptr<sym_t>>> tmp;
        std::weak_ptr<sym_func_t> current_func;
    };
}

#endif //CLIBPARSER_CGEN_H
