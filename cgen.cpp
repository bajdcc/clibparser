//
// Project: clibparser
// Created by bajdcc
//

#include <iostream>
#include <iterator>
#include <unordered_set>
#include <iomanip>
#include "cgen.h"
#include "cast.h"
#include "cvm.h"
#include "cexception.h"

#define AST_IS_KEYWORD(node) ((node)->flag == ast_keyword)
#define AST_IS_KEYWORD_N(node, k) (AST_IS_KEYWORD(node) && (node)->data._keyword == (k))
#define AST_IS_OP(node) ((node)->flag == ast_operator)
#define AST_IS_OP_K(node, k) ((node)->data._op == (k))
#define AST_IS_OP_N(node, k) (AST_IS_OP(node) && AST_IS_OP_K(node, k))
#define AST_IS_ID(node) ((node)->flag == ast_literal)
#define AST_IS_COLL(node) ((node)->flag == ast_collection)
#define AST_IS_COLL_K(node, k) ((node)->data._coll == (k))
#define AST_IS_COLL_N(node, k) (AST_IS_COLL(node) && AST_IS_COLL_K(node, k))

#define LOG_TYPE 1

namespace clib {

    template<class T>
    std::string string_join(const std::vector<T> &v, const string_t &sep) {
        if (v.empty())
            return "";
        std::stringstream ss;
        ss << v[0]->to_string();
        for (size_t i = 1; i < v.size(); ++i) {
            ss << sep;
            ss << v[i]->to_string();
        }
        return std::move(ss.str());
    }

    static int align4(int n) {
        if (n % 4 == 0)
            return n;
        return (n | 3) + 1;
    }

    static ast_node *ast_get_parent(ast_node *node, int level) {
        for (int i = 0; i < level; ++i) {
            if (AST_IS_COLL_N(node, c_program)) {
                break;
            }
            node = node->parent;
        }
        return node;
    }

    int sym_t::size(sym_size_t t) const {
        assert(!"invalid size");
        return 0;
    }

    symbol_t sym_t::get_type() const {
        return s_sym;
    }

    symbol_t sym_t::get_base_type() const {
        return s_sym;
    }

    string_t sym_t::get_name() const {
        return "[Unknown]";
    }

    string_t sym_t::to_string() const {
        return "[Symbol]";
    }

    gen_t sym_t::gen_lvalue(igen &gen) {
        return g_ok;
    }

    gen_t sym_t::gen_rvalue(igen &gen) {
        return g_ok;
    }

    type_t::type_t(int ptr) : ptr(ptr) {}

    symbol_t type_t::get_type() const {
        return s_type;
    }

    symbol_t type_t::get_base_type() const {
        return s_type;
    }

    type_t::ref type_t::clone() const {
        assert(!"invalid clone");
        return nullptr;
    }

    type_base_t::type_base_t(lexer_t type, int ptr) : type_t(ptr), type(type) {}

    int type_base_t::size(sym_size_t t) const {
        if (t == x_inc) {
            if (ptr == 0)
                return 1;
            if (ptr == 1)
                return LEX_SIZE(type);
            return sizeof(void*);
        }
        if (ptr > 0)
            return sizeof(void *);
        return LEX_SIZE(type);
    }

    symbol_t type_base_t::get_type() const {
        return s_type_base;
    }

    string_t type_base_t::get_name() const {
        return LEX_STRING(type);
    }

    string_t type_base_t::to_string() const {
        std::stringstream ss;
        ss << LEX_STRING(type);
        for (int i = 0; i < ptr; ++i) {
            ss << '*';
        }
        return ss.str();
    }

    type_t::ref type_base_t::clone() const {
        return std::make_shared<type_base_t>(type, ptr);
    }

    type_typedef_t::type_typedef_t(const sym_t::ref &refer, int ptr) : type_t(ptr), refer(refer) {}

    symbol_t type_typedef_t::get_type() const {
        return s_type_typedef;
    }

    int type_typedef_t::size(sym_size_t t) const {
        if (ptr > 0)
            return sizeof(void *);
        return refer.lock()->size(t);
    }

    string_t type_typedef_t::to_string() const {
        std::stringstream ss;
        ss << refer.lock()->get_name();
        for (int i = 0; i < ptr; ++i) {
            ss << '*';
        }
        return ss.str();
    }

    type_t::ref type_typedef_t::clone() const {
        return std::make_shared<type_typedef_t>(refer.lock(), ptr);
    }

    sym_id_t::sym_id_t(const type_t::ref &base, const string_t &id)
            : base(base), id(id) {}

    symbol_t sym_id_t::get_type() const {
        return s_id;
    }

    int sym_id_t::size(sym_size_t t) const {
        return base->size(t);
    }

    symbol_t sym_id_t::get_base_type() const {
        return s_id;
    }

    string_t sym_id_t::get_name() const {
        return id;
    }

    string_t sym_id_t::to_string() const {
        std::stringstream ss;
        ss << base->to_string() << " " << id << ", ";
        ss << "Class: " << sym_class_string(clazz) << ", ";
        if (init)
            ss << "Init: " << init->to_string() << ", ";
        ss << "Addr: " << addr;
        return ss.str();
    }

    gen_t sym_id_t::gen_lvalue(igen &gen) {
        if (clazz == z_global_var) {
            gen.emit(IMM, DATA_BASE | addr);
        } else if (clazz == z_local_var) {
            gen.emit(LEA, addr);
        }
        return g_ok;
    }

    gen_t sym_id_t::gen_rvalue(igen &gen) {
        if (clazz == z_global_var) {
            gen.emit(IMM, DATA_BASE | addr);
            if (base->ptr == 0)
                gen.emit(LOAD, base->size(x_size));
        } else if (clazz == z_local_var) {
            gen.emit(LEA, addr);
            gen.emit(LOAD, base->size(x_size));
        }
        return g_ok;
    }

    sym_struct_t::sym_struct_t(const string_t &id) : id(id) {}

    symbol_t sym_struct_t::get_type() const {
        return s_struct;
    }

    symbol_t sym_struct_t::get_base_type() const {
        return s_struct;
    }

    int sym_struct_t::size(sym_size_t t) const {
        if (_size == 0) {
            for (auto &decl : decls) {
                *const_cast<int *>(&_size) += decl->size(t);
            }
        }
        return _size;
    }

    string_t sym_struct_t::get_name() const {
        return id;
    }

    string_t sym_struct_t::to_string() const {
        std::stringstream ss;
        ss << "struct " << id << ", ";
        ss << "decls: [" << string_join(decls, "; ") << "], ";
        return ss.str();
    }

    sym_func_t::sym_func_t(const type_t::ref &base, const string_t &id) : sym_id_t(base, id) {}

    symbol_t sym_func_t::get_type() const {
        return s_function;
    }

    symbol_t sym_func_t::get_base_type() const {
        return s_function;
    }

    int sym_func_t::size(sym_size_t t) const {
        return sizeof(void *);
    }

    string_t sym_func_t::to_string() const {
        std::stringstream ss;
        ss << base->to_string() << " " << id << ", ";
        ss << "Param: [" << string_join(params, "; ") << "], ";
        ss << "Class: " << sym_class_string(clazz) << ", ";
        ss << "Addr: " << addr;
        return ss.str();
    }

    type_exp_t::type_exp_t(const type_t::ref &base) : base(base) {}

    symbol_t type_exp_t::get_type() const {
        return s_expression;
    }

    symbol_t type_exp_t::get_base_type() const {
        return s_expression;
    }

    sym_var_t::sym_var_t(const type_t::ref &base, ast_node *node) : type_exp_t(base), node(node) {
        line = node->line;
        column = node->column;
    }

    symbol_t sym_var_t::get_type() const {
        return s_var;
    }

    int sym_var_t::size(sym_size_t t) const {
        return base->size(t);
    }

    string_t sym_var_t::get_name() const {
        return to_string();
    }

    string_t sym_var_t::to_string() const {
        std::stringstream ss;
        ss << "(type: " << base->to_string() << ", " << cast::to_string(node) << ')';
        return ss.str();
    }

    gen_t sym_var_t::gen_lvalue(igen &gen) {
        return g_ok;
    }

    gen_t sym_var_t::gen_rvalue(igen &gen) {
        switch ((ast_t) node->flag) {
#define DEFINE_VAR(t) \
            case ast_##t: \
                gen.emit(IMM, (LEX_T(int))(node->data._##t)); \
                break;
            DEFINE_VAR(char)
            DEFINE_VAR(uchar)
            DEFINE_VAR(short)
            DEFINE_VAR(ushort)
            DEFINE_VAR(int)
            DEFINE_VAR(uint)
            DEFINE_VAR(float)
#undef DEFINE_VAR
            case ast_long:
            case ast_ulong:
            case ast_double:
                gen.emit(IMX, node->data._ins._1, node->data._ins._2); // 载入8字节
                break;
            case ast_string:
                gen.emit(IMM, DATA_BASE | gen.load_string(node->data._string));
                break;
            default:
                gen.error("sym_var_t::gen_rvalue unsupported type");
                break;
        }
        return g_ok;
    }

    sym_var_id_t::sym_var_id_t(const type_t::ref &base, ast_node *node, const sym_t::ref &symbol)
            : sym_var_t(base, node), id(symbol) {}

    symbol_t sym_var_id_t::get_type() const {
        return s_var_id;
    }

    int sym_var_id_t::size(sym_size_t t) const {
        return id.lock()->size(t);
    }

    string_t sym_var_id_t::get_name() const {
        return id.lock()->get_name();
    }

    string_t sym_var_id_t::to_string() const {
        return id.lock()->to_string();
    }

    gen_t sym_var_id_t::gen_lvalue(igen &gen) {
        return id.lock()->gen_lvalue(gen);
    }

    gen_t sym_var_id_t::gen_rvalue(igen &gen) {
        return id.lock()->gen_rvalue(gen);
    }

    sym_unop_t::sym_unop_t(const type_exp_t::ref &exp, ast_node *op)
            : type_exp_t(nullptr), exp(exp), op(op) {
        line = exp->line;
        column = exp->column;
    }

    symbol_t sym_unop_t::get_type() const {
        return s_unop;
    }

    int sym_unop_t::size(sym_size_t t) const {
        if (t == x_inc)
            return 0;
        return exp->size(t);
    }

    string_t sym_unop_t::get_name() const {
        return to_string();
    }

    string_t sym_unop_t::to_string() const {
        std::stringstream ss;
        ss << "(unop, " << cast::to_string(op) << ", exp: " << exp->to_string() << ')';
        return ss.str();
    }

    gen_t sym_unop_t::gen_lvalue(igen &gen) {
        return g_ok;
    }

    gen_t sym_unop_t::gen_rvalue(igen &gen) {
        switch (op->data._op) {
            case op_plus:
                exp->gen_rvalue(gen);
                break;
            case op_minus:
                exp->gen_rvalue(gen);
                gen.emit(IMM, -1);
                gen.emit(PUSH);
                exp->gen_rvalue(gen);
                gen.emit(MUL);
                break;
            case op_plus_plus:
            case op_minus_minus: {
                exp->gen_lvalue(gen);
                gen.emit(PUSH);
                exp->gen_rvalue(gen);
                gen.emit(PUSH);
                auto inc = exp->size(x_inc);
                gen.emit(IMM, std::max(inc, 1));
                gen.emit(OP_INS(op->data._op));
                gen.emit(SAVE, exp->size(x_size));
                base = exp->base->clone();
                return g_ok;
            }
            case op_logical_not:
                exp->gen_rvalue(gen);
                gen.emit(PUSH);
                gen.emit(IMM, 0);
                gen.emit(EQ);
                break;
            case op_bit_not:
                exp->gen_rvalue(gen);
                gen.emit(PUSH);
                gen.emit(IMM, -1);
                gen.emit(XOR);
                break;
            case op_bit_and: // 取地址
                exp->gen_lvalue(gen);
                break;
            case op_times: // 解引用
                exp->gen_rvalue(gen);
                if (exp->base->to_string().back() != '*')
                    gen.error("invalid deref");
                gen.emit(LOAD, std::max(exp->size(x_inc), 1));
                break;
            default:
                gen.error("unsupported unop");
                break;
        }
    }

    sym_sinop_t::sym_sinop_t(const type_exp_t::ref &exp, ast_node *op)
            : type_exp_t(nullptr), exp(exp), op(op) {
        line = exp->line;
        column = exp->column;
    }

    symbol_t sym_sinop_t::get_type() const {
        return s_sinop;
    }

    int sym_sinop_t::size(sym_size_t t) const {
        if (t == x_inc)
            return 0;
        return exp->size(t);
    }

    string_t sym_sinop_t::get_name() const {
        return to_string();
    }

    string_t sym_sinop_t::to_string() const {
        std::stringstream ss;
        ss << "(sinop, " << cast::to_string(op) << ", exp: " << exp->to_string() << ')';
        return ss.str();
    }

    gen_t sym_sinop_t::gen_lvalue(igen &gen) {
        return g_ok;
    }

    gen_t sym_sinop_t::gen_rvalue(igen &gen) {
        switch (op->data._op) {
            case op_plus_plus:
            case op_minus_minus: {
                exp->gen_rvalue(gen);
                gen.emit(PUSH);
                exp->gen_lvalue(gen);
                gen.emit(PUSH);
                exp->gen_rvalue(gen);
                gen.emit(PUSH);
                auto inc = exp->size(x_inc);
                gen.emit(IMM, std::max(inc, 1));
                gen.emit(OP_INS(op->data._op));
                gen.emit(SAVE, exp->size(x_size));
                base = exp->base->clone();
                gen.emit(POP);
            }
                break;
            default:
                gen.error("unsupported sinop");
                break;
        }
        return g_ok;
    }

    sym_binop_t::sym_binop_t(const type_exp_t::ref &exp1, const type_exp_t::ref &exp2, ast_node *op)
            : type_exp_t(nullptr), exp1(exp1), exp2(exp2), op(op) {
        line = exp1->line;
        column = exp1->column;
    }

    symbol_t sym_binop_t::get_type() const {
        return s_binop;
    }

    int sym_binop_t::size(sym_size_t t) const {
        if (t == x_inc)
            return 0;
        return std::max(exp1->size(x_size), exp2->size(x_size));
    }

    string_t sym_binop_t::get_name() const {
        return to_string();
    }

    string_t sym_binop_t::to_string() const {
        std::stringstream ss;
        ss << "(binop, " << cast::to_string(op)
           << ", exp1: " << exp1->to_string()
           << ", exp2: " << exp2->to_string() << ')';
        return ss.str();
    }

    gen_t sym_binop_t::gen_lvalue(igen &gen) {
        return g_ok;
    }

    gen_t sym_binop_t::gen_rvalue(igen &gen) {
        switch (op->data._op) {
            case op_equal:
            case op_plus:
            case op_minus:
            case op_times:
            case op_divide:
            case op_bit_and:
            case op_bit_or:
            case op_bit_xor:
            case op_mod:
            case op_less_than:
            case op_less_than_or_equal:
            case op_greater_than:
            case op_greater_than_or_equal:
            case op_not_equal:
            case op_left_shift:
            case op_right_shift: {
                exp1->gen_rvalue(gen); // exp1
                gen.emit(PUSH);
                exp2->gen_rvalue(gen); // exp2
                base = exp1->base->clone();
                if (op->data._op == op_plus || op->data._op == op_minus) {
                    auto inc = exp1->size(x_inc);
                    if (inc > 1 && exp2->base->to_string() == "int") { // 指针+常量
                        gen.emit(PUSH);
                        exp2->gen_rvalue(gen);
                        gen.emit(MUL);
                    }
                }
                gen.emit(OP_INS(op->data._op));
            }
                break;
            case op_assign: {
                exp1->gen_lvalue(gen);
                gen.emit(PUSH);
                exp2->gen_rvalue(gen);
                gen.emit(SAVE, exp1->size(x_size));
            }
                break;
            case op_plus_assign:
            case op_minus_assign:
            case op_times_assign:
            case op_div_assign:
            case op_and_assign:
            case op_or_assign:
            case op_xor_assign:
            case op_mod_assign:
            case op_left_shift_assign:
            case op_right_shift_assign: {
                exp1->gen_lvalue(gen);
                gen.emit(PUSH);
                exp1->gen_rvalue(gen);
                gen.emit(PUSH);
                exp2->gen_rvalue(gen);
                gen.emit(OP_INS(op->data._op));
                gen.emit(SAVE, exp1->size(x_size));
            }
                break;
            default:
                break;
        }
        return g_ok;
    }

    sym_triop_t::sym_triop_t(const type_exp_t::ref &exp1, const type_exp_t::ref &exp2,
                             const type_exp_t::ref &exp3, ast_node *op1, ast_node *op2)
            : type_exp_t(nullptr), exp1(exp1), exp2(exp2), exp3(exp3), op1(op1), op2(op2) {
        line = exp1->line;
        column = exp1->column;
    }

    symbol_t sym_triop_t::get_type() const {
        return s_triop;
    }

    int sym_triop_t::size(sym_size_t t) const {
        if (t == x_inc)
            return 0;
        return std::max(std::max(exp1->size(x_size), exp2->size(x_size)), exp3->size(x_size));
    }

    string_t sym_triop_t::get_name() const {
        return to_string();
    }

    string_t sym_triop_t::to_string() const {
        std::stringstream ss;
        ss << "(triop, " << cast::to_string(op1)
           << ", " << cast::to_string(op2)
           << ", exp1: " << exp1->to_string()
           << ", exp2: " << exp2->to_string()
           << ", exp3: " << exp3->to_string() << ')';
        return ss.str();
    }

    symbol_t sym_list_t::get_type() const {
        return s_list;
    }

    int sym_list_t::size(sym_size_t t) const {
        if (t == x_inc)
            return 0;
        return exps.size();
    }

    string_t sym_list_t::get_name() const {
        return to_string();
    }

    string_t sym_list_t::to_string() const {
        std::stringstream ss;
        ss << "(list, " << string_join(exps, ", ") << ')';
        return ss.str();
    }

    sym_list_t::sym_list_t() : type_exp_t(nullptr) {}

    // --------------------------------------------------------------

    cgen::cgen() {
        reset();
    }

    void cgen::gen(ast_node *node) {
        gen_rec(node, 0);
    }

    void cgen::reset() {
        symbols.clear();
        symbols.emplace_back();
        tmp.clear();
        tmp.emplace_back();
        ast.clear();
        ast.emplace_back();
    }

    void cgen::eval() {
        auto entry = symbols[0].find("main");
        if (entry == symbols[0].end()) {
            error("main() not defined");
        }
        cvm vm(text, data);
        vm.exec(std::dynamic_pointer_cast<sym_func_t>(entry->second)->addr);
    }

    void cgen::emit(ins_t i) {
        text.push_back(i);
#if LOG_TYPE
        std::cout << "[DEBUG] *GEN* ==> " << INS_STRING(i) << std::endl;
#endif
    }

    void cgen::emit(ins_t i, int d) {
        text.push_back(i);
        text.push_back(d);
#if LOG_TYPE
        std::cout << "[DEBUG] *GEN* ==> " << INS_STRING(i) << " " <<
                  std::setiosflags(std::ios::uppercase) << std::hex << d << std::endl;
#endif
    }

    void cgen::emit(ins_t i, int d, int e) {
        text.push_back(i);
        text.push_back(d);
#if LOG_TYPE
        std::cout << "[DEBUG] *GEN* ==> " << INS_STRING(i) <<
                  " " << std::setiosflags(std::ios::uppercase) << std::hex <<
                  d << " " << e << std::endl;
#endif
    }

    int cgen::load_string(const string_t &s) {
        auto addr = data.size();
        std::copy(s.begin(), s.end(), std::back_inserter(data));
        data.push_back(0);
        while (data.size() % 4 != 0) {
            data.push_back(0);
        }
#if LOG_TYPE
        std::cout << "[DEBUG] *GEN* <== STRING: \"" << cast::display_str(s.c_str())
                  << "\", addr: " << addr << ", size: " << s.length() << std::endl;
#endif
        return addr;
    }

    template<class T>
    static void gen_recursion(ast_node *node, int level, T f) {
        if (node == nullptr)
            return;
        auto i = node;
        if (i->next == i) {
            f(i, level);
            return;
        }
        f(i, level);
        i = i->next;
        while (i != node) {
            f(i, level);
            i = i->next;
        }
    }

    template<class T>
    static std::vector<T *> gen_get_children(T *node) {
        std::vector<T *> v;
        if (node == nullptr)
            return v;
        auto i = node;
        if (i->next == i) {
            v.push_back(i);
            return v;
        }
        v.push_back(i);
        i = i->next;
        while (i != node) {
            v.push_back(i);
            i = i->next;
        }
        return v;
    }

    void cgen::gen_rec(ast_node *node, int level) {
        if (node == nullptr)
            return;
        auto rec = [this](auto n, auto l) { this->gen_rec(n, l); };
        auto type = (ast_t) node->flag;
        if (type == ast_collection) {
            if ((node->attr & a_exp) && node->child == node->child->next) {
                gen_recursion(node->child, level, rec);
                return;
            }
        }
        tmp.emplace_back();
        ast.emplace_back();
        switch (type) {
            case ast_root: {// 根结点，全局声明
                gen_recursion(node->child, level, rec);
            }
                break;
            case ast_collection: {
                auto children = gen_get_children(node->child);
                gen_coll(children, level + 1, node);
            }
                break;
            case ast_keyword:
            case ast_operator:
            case ast_literal:
            case ast_string:
            case ast_char:
            case ast_uchar:
            case ast_short:
            case ast_ushort:
            case ast_int:
            case ast_uint:
            case ast_long:
            case ast_ulong:
            case ast_float:
            case ast_double:
                ast.back().push_back(node);
                break;
        }
        if (!tmp.back().empty()) {
            //assert(tmp.back().size() == 1);
            auto &top = tmp[tmp.size() - 2];
            for (auto &t : tmp.back()) {
                top.push_back(t);
            }
        }
        tmp.pop_back();
        if (!ast.back().empty()) {
            auto &top = ast[ast.size() - 2];
            std::copy(ast.back().begin(), ast.back().end(), std::back_inserter(top));
        }
        ast.pop_back();
    }

    void cgen::gen_coll(const std::vector<ast_node *> &nodes, int level, ast_node *node) {
        switch (node->data._coll) {
            case c_program:
                break;
            case c_primaryExpression:
                break;
            case c_constant:
                break;
            case c_postfixExpression:
                break;
            case c_argumentExpressionList:
                break;
            case c_unaryExpression:
                break;
            case c_unaryOperator:
                break;
            case c_castExpression:
                break;
            case c_multiplicativeExpression:
                break;
            case c_additiveExpression:
                break;
            case c_shiftExpression:
                break;
            case c_relationalExpression:
                break;
            case c_equalityExpression:
                break;
            case c_andExpression:
                break;
            case c_exclusiveOrExpression:
                break;
            case c_inclusiveOrExpression:
                break;
            case c_logicalAndExpression:
                break;
            case c_logicalOrExpression:
                break;
            case c_conditionalExpression:
                break;
            case c_assignmentExpression:
                break;
            case c_assignmentOperator:
                break;
            case c_expression:
                break;
            case c_constantExpression:
                break;
            case c_declaration:
                break;
            case c_declarationSpecifiers:
                break;
            case c_declarationSpecifiers2:
                break;
            case c_declarationSpecifier:
                break;
            case c_initDeclaratorList:
                break;
            case c_initDeclarator:
                break;
            case c_storageClassSpecifier:
                break;
            case c_typeSpecifier:
                break;
            case c_structOrUnion:
                break;
            case c_structDeclarationList: {
                auto name = (ast.rbegin() + 1)->back();
                auto &sym = symbols[0];
                auto f = sym.find(name->data._string);
                if (f != sym.end()) {
                    ctx = f->second;
                } else {
                    error("invalid struct name");
                }
            }
                break;
            case c_structDeclaration:
                break;
            case c_specifierQualifierList:
                break;
            case c_structDeclaratorList:
                break;
            case c_structDeclarator:
                break;
            case c_enumSpecifier:
                break;
            case c_enumeratorList:
                break;
            case c_enumerator:
                break;
            case c_enumerationConstant:
                break;
            case c_typeQualifier:
                break;
            case c_declarator:
                break;
            case c_directDeclarator:
                break;
            case c_pointer:
                break;
            case c_typeQualifierList:
                break;
            case c_parameterTypeList:
                break;
            case c_parameterList:
                break;
            case c_parameterDeclaration:
                break;
            case c_identifierList:
                break;
            case c_typeName:
                break;
            case c_abstractDeclarator:
                break;
            case c_directAbstractDeclarator:
                break;
            case c_typedefName:
                break;
            case c_initializer:
                break;
            case c_initializerList:
                break;
            case c_designation:
                break;
            case c_designatorList:
                break;
            case c_designator:
                break;
            case c_statement:
                break;
            case c_labeledStatement:
                break;
            case c_compoundStatement:
                break;
            case c_blockItemList:
                break;
            case c_blockItem:
                break;
            case c_expressionStatement:
                break;
            case c_selectionStatement:
                break;
            case c_iterationStatement:
                break;
            case c_forCondition:
                break;
            case c_forDeclaration:
                break;
            case c_forExpression:
                break;
            case c_jumpStatement:
                break;
            case c_compilationUnit:
                break;
            case c_translationUnit:
                break;
            case c_externalDeclaration:
                break;
            case c_functionDefinition:
            case c_structOrUnionSpecifier:
                symbols.emplace_back();
                break;
            case c_declarationList:
                break;
        }
        for (auto &n : nodes) {
            gen_rec(n, level);
        }
        auto &asts = ast.back();
        switch (node->data._coll) {
            case c_program:
                break;
            case c_primaryExpression: {
                if (tmp.back().empty()) {
                    auto pri = primary_node(asts[0]);
                    tmp.back().push_back(pri);
                    asts.clear();
                }
            }
                break;
            case c_constant:
                break;
            case c_postfixExpression: {
                if (AST_IS_COLL_N(nodes[0], c_primaryExpression)) {
                    auto &_tmp = tmp.back();
                    auto tmp_i = 0;
                    auto exp = to_exp(_tmp[tmp_i++]);
                    for (int i = 1; i < nodes.size(); ++i) {
                        auto &a = nodes[i];
                        if (AST_IS_OP(a)) {
                            if (AST_IS_OP_K(a, op_plus_plus) || AST_IS_OP_K(a, op_minus_minus)) {
                                exp = std::make_shared<sym_sinop_t>(exp, a);
                            } else if (AST_IS_OP_K(a, op_dot) || AST_IS_OP_K(a, op_pointer)) {
                                ++i;
                                auto exp2 = primary_node(nodes[i]);
                                exp = std::make_shared<sym_binop_t>(exp, exp2, a);
                            } else if (AST_IS_OP_K(a, op_lsquare)) {
                                ++i;
                                auto exp2 = to_exp(_tmp[tmp_i++]);
                                exp = std::make_shared<sym_binop_t>(exp, exp2, a);
                            } else if (AST_IS_OP_K(a, op_lparan)) {
                                ++i;
                                auto exp2 = std::make_shared<sym_list_t>();
                                while (true) {
                                    if (AST_IS_OP_K(nodes[i], op_rparan))
                                        break;
                                    exp2->exps.push_back(to_exp(_tmp[tmp_i++]));
                                    ++i;
                                }
                                exp = std::make_shared<sym_binop_t>(exp, exp2, a);
                            } else {
                                error("invalid postfix exp: op");
                            }
                        } else {
                            error("invalid postfix exp: coll");
                        }
                    }
                    _tmp.clear();
                    _tmp.push_back(exp);
                    asts.clear();
                }
            }
                break;
            case c_argumentExpressionList:
                break;
            case c_unaryExpression: {
                auto &op = asts[0];
                if (AST_IS_OP(op)) {
                    switch (op->data._op) {
                        case op_plus:
                        case op_plus_plus:
                        case op_minus:
                        case op_minus_minus:
                        case op_logical_and:
                        case op_logical_not:
                        case op_bit_and:
                        case op_times: {
                            auto &_exp = tmp.back().back();
                            auto exp = to_exp(_exp);
                            tmp.back().clear();
                            auto unop = std::make_shared<sym_unop_t>(exp, op);
                            tmp.back().push_back(unop);
                            asts.clear();
                        }
                            break;
                        default:
                            error("invalid unary exp: op");
                            break;
                    }
                } else {
                    error("invalid unary exp: coll");
                }
            }
                break;
            case c_unaryOperator:
                break;
            case c_castExpression:
                break;
            case c_multiplicativeExpression:
            case c_additiveExpression:
            case c_shiftExpression:
            case c_relationalExpression:
            case c_equalityExpression:
            case c_andExpression:
            case c_exclusiveOrExpression:
            case c_inclusiveOrExpression:
            case c_logicalAndExpression:
            case c_logicalOrExpression:
            case c_conditionalExpression: {
                auto &_tmp = tmp.back();
                auto tmp_i = 0;
                auto exp1 = to_exp(_tmp[tmp_i++]);
                auto exp2 = to_exp(_tmp[tmp_i++]);
                for (auto &a : asts) {
                    if (AST_IS_OP(a)) {
                        exp1 = std::make_shared<sym_binop_t>(exp1, exp2, a);
                        if (tmp_i < _tmp.size())
                            exp2 = to_exp(_tmp[tmp_i++]);
                    } else {
                        error("invalid plus/minus exp: coll");
                    }
                }
                _tmp.clear();
                _tmp.push_back(exp1);
                asts.clear();
            }
                break;
            case c_assignmentExpression: {
                auto &_tmp = tmp.back();
                auto tmp_i = 0;
                auto exp1 = to_exp(_tmp[tmp_i++]);
                auto exp2 = to_exp(_tmp[tmp_i++]);
                auto exp = std::make_shared<sym_binop_t>(exp1, exp2, asts.front());
                _tmp.clear();
                _tmp.push_back(exp);
                asts.clear();
            }
                break;
            case c_assignmentOperator:
                break;
            case c_expression:
                break;
            case c_constantExpression:
                break;
            case c_declaration:
                break;
            case c_declarationSpecifiers:
            case c_specifierQualifierList: {
                type_t::ref base_type;
                if (AST_IS_KEYWORD_N(asts[0], k_struct)) {
                    asts.clear();
                    break;
                }
                if (AST_IS_KEYWORD_N(asts[0], k_unsigned)) { // unsigned ...
                    if (asts.size() == 1 || (asts.size() > 1 && !AST_IS_KEYWORD(asts[1]))) {
                        base_type = std::make_shared<type_base_t>(l_int);
                        base_type->line = asts[0]->line;
                        base_type->column = asts[0]->column;
                        asts.erase(asts.begin());
                    } else {
                        assert(asts.size() > 1 && AST_IS_KEYWORD(asts[1]));
                        lexer_t type;
                        switch (asts[1]->data._keyword) {
                            case k_char:
                                type = l_uchar;
                                break;
                            case k_short:
                                type = l_short;
                                break;
                            case k_int:
                                type = l_uint;
                                break;
                            case k_long:
                                type = l_ulong;
                                break;
                            default:
                                error(asts[1], "invalid unsigned * get_type");
                                break;
                        }
                        base_type = std::make_shared<type_base_t>(type);
                        base_type->line = asts[0]->line;
                        base_type->column = asts[0]->column;
                        asts.erase(asts.begin());
                        asts.erase(asts.begin());
                    }
                } else {
                    auto type = l_none;
                    switch (asts[0]->data._keyword) {
                        case k_char:
                            type = l_char;
                            break;
                        case k_double:
                            type = l_double;
                            break;
                        case k_float:
                            type = l_float;
                            break;
                        case k_int:
                            type = l_int;
                            break;
                        case k_long:
                            type = l_long;
                            break;
                        case k_short:
                            type = l_short;
                            break;
                        default:
                            break;
                    }
                    if (type != l_none) {
                        base_type = std::make_shared<type_base_t>(type);
                        base_type->line = asts[0]->line;
                        base_type->column = asts[0]->column;
                        asts.erase(asts.begin());
                    } else {
                        if (AST_IS_ID(asts[0])) {
                            auto &sym = symbols[0];
                            auto f = sym.find(asts[0]->data._string);
                            if (f != sym.end()) {
                                auto &typedef_name = f->second;
                                auto t = typedef_name->get_base_type();
                                if (t == s_type || t == s_struct || t == s_function) {
                                    base_type = std::make_shared<type_typedef_t>(typedef_name);
                                    base_type->line = asts[0]->line;
                                    base_type->column = asts[0]->column;
                                    asts.erase(asts.begin());
                                } else {
                                    error(asts[0], "invalid typedef name");
                                }
                            } else {
                                error(asts[0], "unknown typedef name");
                            }
                        } else {
                            error(asts[0], "invalid * get_type");
                        }
                    }
                }
                if (!asts.empty()) {
                    auto ptr = 0;
                    for (auto &a : asts) {
                        if (AST_IS_OP_N(a, op_times)) {
                            ptr++;
                        } else {
                            break;
                        }
                    }
                    base_type->ptr = ptr;
                    asts.erase(asts.begin(), asts.begin() + ptr);
                }
#if LOG_TYPE
                std::cout << "[DEBUG] Type: " << base_type->to_string() << std::endl;
#endif
                tmp.back().push_back(base_type);
            }
                break;
            case c_declarationSpecifiers2:
                break;
            case c_declarationSpecifier:
                break;
            case c_initDeclaratorList:
            case c_structDeclaratorList: {
                decltype(z_undefined) clazz;
                if (AST_IS_COLL_N(node, c_initDeclaratorList)) {
                    if (ctx.lock()) {
                        clazz = z_local_var;
                    } else {
                        clazz = z_global_var;
                    }
                } else {
                    clazz = z_struct_var;
                }
                auto type = std::dynamic_pointer_cast<type_t>((tmp.rbegin() + 1)->front());
                auto ptr = 0;
                auto tmp_i = 0;
                auto &_tmp = tmp.back();
                for (auto &ast : asts) {
                    if (AST_IS_OP_N(ast, op_times)) {
                        ptr++;
                    } else {
                        auto new_type = type->clone();
                        new_type->ptr = ptr;
                        type_exp_t::ref init;
                        if (clazz != z_struct_var) {
                            auto t = _tmp[tmp_i++];
                            if (t) {
                                init = to_exp(t);
                            }
                        }
                        auto new_id = add_id(new_type, clazz, ast, init);
                        ptr = 0;
                    }
                }
                _tmp.clear();
            }
                break;
            case c_initDeclarator: {
                if (tmp.back().empty())
                    tmp.back().push_back(nullptr);
            }
                break;
            case c_storageClassSpecifier:
                break;
            case c_typeSpecifier:
                break;
            case c_structOrUnion:
                break;
            case c_structDeclarationList:
                break;
            case c_structDeclaration:
                break;
            case c_structDeclarator:
                break;
            case c_enumSpecifier:
                break;
            case c_enumeratorList:
                break;
            case c_enumerator:
                break;
            case c_enumerationConstant:
                break;
            case c_typeQualifier:
                break;
            case c_declarator:
                break;
            case c_directDeclarator: {
                if (AST_IS_OP_N(node->child->next, op_lparan)) {
                    auto has_impl = AST_IS_COLL_N(node->parent->parent, c_functionDefinition);
                    type_t::ref type;
                    for (auto t = tmp.rbegin() + 1; t != tmp.rend(); t++) {
                        if (!t->empty()) {
                            auto tt = t->front();
                            if (tt && tt->get_base_type() == s_type) {
                                type = std::dynamic_pointer_cast<type_t>(t->front());
                                break;
                            }
                        }
                    }
                    assert(type);
                    if (AST_IS_COLL_N(node->parent->child, c_pointer)) {
                        auto children = gen_get_children(node->parent->child->child);
                        for (auto &child : children) {
                            assert(AST_IS_OP_N(child, op_times));
                        }
                        type->ptr = children.size();
                    }
                    auto func = std::make_shared<sym_func_t>(type, nodes[0]->data._string);
                    ctx = func;
                    func->clazz = z_function;
                    func->addr = text.size();
                    symbols[0].insert(std::make_pair(nodes[0]->data._string, func));
                    auto &tmps = tmp.back();
                    std::unordered_set<string_t> ids;
                    auto ptr = 0;
                    for (auto i = 2, j = 0; i < asts.size() && j < tmp.size(); ++i) {
                        auto &pa = asts[i];
                        if (AST_IS_ID(pa)) {
                            const auto &pt = std::dynamic_pointer_cast<type_t>(tmps[j]);
                            pt->ptr = ptr;
                            auto &name = pa->data._string;
                            auto id = std::make_shared<sym_id_t>(pt, name);
                            id->line = pa->line;
                            id->column = pa->column;
                            id->clazz = z_param_var;
                            allocate(id, nullptr);
                            func->params.push_back(id);
                            if (!ids.insert(name).second) {
                                error(id, "conflict id: " + id->to_string());
                            }
                            {
                                auto f = symbols[0].find(pa->data._string);
                                if (f != symbols[0].end()) {
                                    if (f->second->get_type() == s_function) {
                                        error(id, "conflict id with function: " + id->to_string());
                                    }
                                }
                            }
                            ptr = 0;
                            j++;
                        } else if (AST_IS_OP_N(pa, op_times)) {
                            ptr++;
                        } else {
                            error(pa, "invalid param: ", true);
                        }
                    }
                    func->ebp += sizeof(void *);
                    func->ebp_local = func->ebp;
                    tmps.clear();
                    asts.clear();
#if LOG_TYPE
                    std::cout << "[DEBUG] Func: " << ctx.lock()->to_string() << std::endl;
#endif
                    if (has_impl) {
                        tmps.push_back(func);
                        emit(ENT, 0);
                        func->entry = text.size() - 1;
                    } else {
                        ctx.reset();
                    }
                }
            }
                break;
            case c_pointer:
                break;
            case c_typeQualifierList:
                break;
            case c_parameterTypeList:
                break;
            case c_parameterList:
                break;
            case c_parameterDeclaration:
                break;
            case c_identifierList:
                break;
            case c_typeName:
                break;
            case c_abstractDeclarator:
                break;
            case c_directAbstractDeclarator:
                break;
            case c_typedefName:
                break;
            case c_initializer:
                break;
            case c_initializerList:
                break;
            case c_designation:
                break;
            case c_designatorList:
                break;
            case c_designator:
                break;
            case c_statement:
                break;
            case c_labeledStatement:
                break;
            case c_compoundStatement:
                break;
            case c_blockItemList:
                break;
            case c_blockItem:
                break;
            case c_expressionStatement: {
#if LOG_TYPE
                std::cout << "[DEBUG] Exp: " << string_join(tmp.back(), ", ") << std::endl;
#endif
                tmp.back().front()->gen_rvalue(*this);
            }
                break;
            case c_selectionStatement:
                break;
            case c_iterationStatement:
                break;
            case c_forCondition:
                break;
            case c_forDeclaration:
                break;
            case c_forExpression:
                break;
            case c_jumpStatement:
                break;
            case c_compilationUnit:
                break;
            case c_translationUnit:
                break;
            case c_externalDeclaration:
                break;
            case c_functionDefinition:
                emit(LEV);
            case c_structOrUnionSpecifier: {
                ctx.reset();
                symbols.pop_back();
            }
                break;
            case c_declarationList:
                break;
        }
    }

    void cgen::error(const string_t &str) {
        std::stringstream ss;
        ss << "GEN ERROR: " << str;
        throw cexception(ss.str());
    }

    void cgen::error(ast_node *node, const string_t &str, bool info) {
        std::stringstream ss;
        ss << "GEN ERROR: " << "[" << node->line << ":" << node->column << "] " << str;
        if (info) {
            cast::print(node, 0, ss);
        }
        throw cexception(ss.str());
    }

    void cgen::error(sym_t::ref s, const string_t &str) {
        std::stringstream ss;
        ss << "GEN ERROR: " << "[" << s->line << ":" << s->column << "] " << str;
        throw cexception(ss.str());
    }

    type_exp_t::ref cgen::to_exp(sym_t::ref s) {
        assert(s->get_base_type() == s_expression);
        return std::dynamic_pointer_cast<type_exp_t>(s);
    }

    void cgen::allocate(sym_id_t::ref id, const type_exp_t::ref &init) {
        assert(id->base->get_base_type() == s_type);
        auto type = id->base;
        if (id->clazz == z_global_var) {
            id->addr = data.size();
            auto size = align4(type->size(x_size));
            if (init) {
                if (init->get_type() == s_var) {
                    auto var = std::dynamic_pointer_cast<sym_var_t>(init);
                    auto &node = var->node;
                    if (node->flag != ast_string) {
                        std::copy((char *) &node->data._ins,
                                  ((char *) &node->data._ins) + size,
                                  std::back_inserter(data));
                    } else {
                        load_string(node->data._string);
                    }
                } else if (init->get_type() == s_var_id) {
                    auto var = std::dynamic_pointer_cast<sym_var_id_t>(init);
                    auto _id = var->id.lock();
                    if (_id->get_type() != s_id)
                        error(id, "allocate: invalid init value");
                    auto var2 = std::dynamic_pointer_cast<sym_id_t>(_id);
                    std::copy(data.data() + var2->addr,
                              data.data() + var2->addr_end,
                              std::back_inserter(data));
                } else {
                    error(id, "allocate: not supported");
                }
            } else {
                for (auto i = 0; i < size; ++i) {
                    data.push_back(0);
                }
            }
            id->addr_end = data.size();
        } else if (id->clazz == z_local_var) {
            auto size = align4(type->size(x_size));
            auto func = std::dynamic_pointer_cast<sym_func_t>(ctx.lock());
            func->ebp_local += size;
            text[func->entry] += size;
            id->addr = func->ebp - func->ebp_local;
            id->addr_end = id->addr - size;
            if (init) {
                auto var = std::dynamic_pointer_cast<sym_var_t>(init);
                // L_VALUE
                // PUSH
                // R_VALUE
                // SAVE
                id->gen_lvalue(*this);
                emit(PUSH);
                var->gen_rvalue(*this);
                emit(SAVE, size);
            }
        } else if (id->clazz == z_param_var) {
            if (init)
                error(id, "allocate: invalid init value");
            auto size = align4(type->size(x_size));
            auto func = std::dynamic_pointer_cast<sym_func_t>(ctx.lock());
            id->addr = func->ebp;
            func->ebp += size;
            id->addr_end = id->addr + size;
        } else if (id->clazz == z_struct_var) {
            if (init)
                error(id, "allocate: invalid init value");
            auto _struct = std::dynamic_pointer_cast<sym_struct_t>(ctx.lock());
            _struct->decls.push_back(id);
        } else {
            error(id, "allocate: not supported");
        }
    }

    sym_id_t::ref cgen::add_id(const type_base_t::ref &type, sym_class_t clazz, ast_node *node, const type_exp_t::ref &init) {
        assert(AST_IS_ID(node));
        auto new_id = std::make_shared<sym_id_t>(type, node->data._string);
        new_id->line = node->line;
        new_id->column = node->column;
        new_id->clazz = clazz;
        new_id->init = init;
        if (init) {
            if (type->to_string() != init->base->to_string())
                error(node, "not equal init type, ", true);
        }
        allocate(new_id, init);
#if LOG_TYPE
        std::cout << "[DEBUG] Id: " << new_id->to_string() << std::endl;
#endif
        if (!symbols.back().insert(std::make_pair(node->data._string, new_id)).second) {
            error(new_id, "conflict id: " + new_id->to_string());
        }
        if (symbols.size() > 1) {
            auto f = symbols[0].find(node->data._string);
            if (f != symbols[0].end()) {
                if (f->second->get_type() == s_function) {
                    error(new_id, "conflict id with function: " + new_id->to_string());
                }
            }
        }
        return new_id;
    }

    sym_t::ref cgen::find_symbol(const string_t &name) {
        for (auto s = symbols.rbegin(); s != symbols.rend(); s++) {
            auto f = s->find(name);
            if (f != s->end()) {
                return f->second;
            }
        }
        return nullptr;
    }

    sym_var_t::ref cgen::primary_node(ast_node *node) {
        type_t::ref t;
        switch (node->flag) {
            case ast_literal: {
                auto sym = find_symbol(node->data._string);
                if (!sym)
                    error(node, "undefined id: " + string_t(node->data._string));
                if (sym->get_type() != s_id)
                    error(node, "required id but got: " + sym->to_string());
                t = std::dynamic_pointer_cast<sym_id_t>(sym)->base->clone();
                return std::make_shared<sym_var_id_t>(t, node, sym);
            }
            case ast_string:
                t = std::make_shared<type_base_t>(l_char, 1);
                break;
#define DEF_LEX(name) \
            case ast_##name: \
                t = std::make_shared<type_base_t>(l_##name, 0); \
                break;
            DEF_LEX(char);
            DEF_LEX(uchar);
            DEF_LEX(short);
            DEF_LEX(ushort);
            DEF_LEX(int);
            DEF_LEX(uint);
            DEF_LEX(long);
            DEF_LEX(ulong);
            DEF_LEX(float);
            DEF_LEX(double);
#undef DEF_LEX
            default:
                assert(!"invalid var type");
                break;
        }
        return std::make_shared<sym_var_t>(t, node);
    }

    std::tuple<sym_class_t, string_t> sym_class_string_list[] = {
            std::make_tuple(z_undefined, "undefined"),
            std::make_tuple(z_global_var, "global id"),
            std::make_tuple(z_local_var, "local id"),
            std::make_tuple(z_param_var, "param id"),
            std::make_tuple(z_struct_var, "struct id"),
            std::make_tuple(z_function, "func id"),
    };

    const string_t &sym_class_string(sym_class_t t) {
        assert(t >= z_undefined && t < z_end);
        return std::get<1>(sym_class_string_list[t]);
    }

    backtrace_direction cgen::check(pda_edge_t edge, ast_node *node) {
        if (edge != e_shift) { // CONTAINS reduce, recursion, move
            if (AST_IS_COLL(node)) {
                switch (node->data._coll) {
                    case c_structOrUnionSpecifier: { // MODIFY, CANNOT RECOVERY
                        if (AST_IS_ID(node->child->next)) {
                            auto id = node->child->next->data._string;
                            auto &sym = symbols[0];
                            if (sym.find(id) != sym.end()) {
                                // CONFLICT STRUCT DECLARATION
                                return b_fail;
                            }
                            sym.insert(std::make_pair(id, std::make_shared<sym_struct_t>(id)));
                        }
                    }
                        break;
                    case c_typedefName: { // READONLY
                        if (AST_IS_ID(node->child)) {
                            auto id = node->child->next->data._string;
                            auto &sym = symbols[0];
                            auto f = sym.find(id);
                            if (f != sym.end()) {
                                auto t = f->second->get_base_type();
                                if (t == s_type || t == s_struct || t == s_function)
                                    return b_next;
                            }
                            return b_error;
                        }
                    }
                        break;
                    default:
                        break;
                }
            }
        }
        return b_next;
    }
}
