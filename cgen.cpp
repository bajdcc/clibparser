//
// Project: clibparser
// Created by bajdcc
//

#include <iostream>
#include <iterator>
#include <unordered_set>
#include "cgen.h"
#include "cast.h"
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

    int sym_t::size() const {
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

    int type_base_t::size() const {
        if (ptr > 0)
            return sizeof(void *);
        switch (type) {
#define DEFINE_LEXER_SIZE(t) case l_##t: return LEX_SIZEOF(t);
            DEFINE_LEXER_SIZE(char)
            DEFINE_LEXER_SIZE(uchar)
            DEFINE_LEXER_SIZE(short)
            DEFINE_LEXER_SIZE(ushort)
            DEFINE_LEXER_SIZE(int)
            DEFINE_LEXER_SIZE(uint)
            DEFINE_LEXER_SIZE(long)
            DEFINE_LEXER_SIZE(ulong)
            DEFINE_LEXER_SIZE(float)
            DEFINE_LEXER_SIZE(double)
            DEFINE_LEXER_SIZE(operator)
            DEFINE_LEXER_SIZE(keyword)
            DEFINE_LEXER_SIZE(identifier)
            DEFINE_LEXER_SIZE(string)
            DEFINE_LEXER_SIZE(comment)
            DEFINE_LEXER_SIZE(space)
            DEFINE_LEXER_SIZE(newline)
            DEFINE_LEXER_SIZE(error)
#undef DEFINE_LEXER_SIZE
            default:
                assert(!"invalid get_type");
                return 0;
        }
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

    int type_typedef_t::size() const {
        if (ptr > 0)
            return sizeof(void *);
        return refer.lock()->size();
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

    int sym_id_t::size() const {
        return base->size();
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
        ss << "Addr: " << addr;
        return ss.str();
    }

    sym_struct_t::sym_struct_t(const string_t &id) : id(id) {}

    symbol_t sym_struct_t::get_type() const {
        return s_struct;
    }

    symbol_t sym_struct_t::get_base_type() const {
        return s_struct;
    }

    int sym_struct_t::size() const {
        if (_size == 0) {
            for (auto &decl : decls) {
                *const_cast<int *>(&_size) += decl->size();
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

    int sym_func_t::size() const {
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

    symbol_t type_exp_t::get_type() const {
        return s_expression;
    }

    symbol_t type_exp_t::get_base_type() const {
        return s_expression;
    }

    sym_var_t::sym_var_t(const type_t::ref &base, ast_node *node) : node(node) {
        this->base = base;
    }

    symbol_t sym_var_t::get_type() const {
        return s_var;
    }

    int sym_var_t::size() const {
        return base->size();
    }

    string_t sym_var_t::get_name() const {
        return to_string();
    }

    string_t sym_var_t::to_string() const {
        std::stringstream ss;
        ss << "(type: " << base->to_string() << ", " << cast::to_string(node) << ')';
        return ss.str();
    }

    sym_unop_t::sym_unop_t(const type_exp_t::ref &exp, ast_node *op) : exp(exp), op(op) {}

    symbol_t sym_unop_t::get_type() const {
        return s_unop;
    }

    int sym_unop_t::size() const {
        return exp->size();
    }

    string_t sym_unop_t::get_name() const {
        return to_string();
    }

    string_t sym_unop_t::to_string() const {
        std::stringstream ss;
        ss << "(unop, " << cast::to_string(op) << ", exp: " << exp->to_string() << ')';
        return ss.str();
    }

    sym_sinop_t::sym_sinop_t(const type_exp_t::ref &exp, ast_node *op) : exp(exp), op(op) {}

    symbol_t sym_sinop_t::get_type() const {
        return s_sinop;
    }

    int sym_sinop_t::size() const {
        return exp->size();
    }

    string_t sym_sinop_t::get_name() const {
        return to_string();
    }

    string_t sym_sinop_t::to_string() const {
        std::stringstream ss;
        ss << "(sinop, " << cast::to_string(op) << ", exp: " << exp->to_string() << ')';
        return ss.str();
    }

    sym_binop_t::sym_binop_t(const type_exp_t::ref &exp1, const type_exp_t::ref &exp2, ast_node *op)
        : exp1(exp1), exp2(exp2), op(op) {}

    symbol_t sym_binop_t::get_type() const {
        return s_binop;
    }

    int sym_binop_t::size() const {
        return std::max(exp1->size(), exp2->size());
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

    sym_triop_t::sym_triop_t(const type_exp_t::ref& exp1, const type_exp_t::ref& exp2,
                             const type_exp_t::ref& exp3, ast_node *op1, ast_node *op2)
                             : exp1(exp1), exp2(exp2), exp3(exp3), op1(op1), op2(op2) {}

    symbol_t sym_triop_t::get_type() const {
        return s_triop;
    }

    int sym_triop_t::size() const {
        return std::max(std::max(exp1->size(), exp2->size()), exp3->size());
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

    int sym_list_t::size() const {
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
                auto pri = primary_node(asts[0]);
                tmp.back().push_back(pri);
                asts.clear();
            }
                break;
            case c_constant:
                break;
            case c_postfixExpression: {
                if (nodes[0]->data._coll == c_primaryExpression) {
                    auto &_tmp = tmp.back();
                    auto tmp_i = 0;
                    auto exp = to_exp(_tmp[tmp_i++]);
                    for (int i = 1; i < nodes.size(); ++i) {
                        auto &a = nodes[i];
                        if (AST_IS_OP(a)) {
                            if (AST_IS_OP_K(a, op_plus_plus) || AST_IS_OP_K(a, op_minus_minus)) {
                                exp = std::make_shared<sym_sinop_t>(exp, a);
                            } else if (AST_IS_OP_K(a, op_dot) ||AST_IS_OP_K(a, op_pointer)) {
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
                }
            }
                break;
            case c_argumentExpressionList:
                break;
            case c_unaryExpression: {
                auto &op = asts[0];
                if (AST_IS_OP(op)) {
                    if (AST_IS_OP_K(op, op_plus) || AST_IS_OP_K(op, op_minus)) {
                        auto &_exp = tmp.back().back();
                        auto exp = to_exp(_exp);
                        tmp.back().pop_back();
                        auto unop = std::make_shared<sym_unop_t>(exp, op);
                        tmp.back().push_back(unop);
                        asts.clear();
                    }
                }
            }
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
                auto clazz = z_undefined;
                if (node->data._coll == c_initDeclaratorList) {
                    if (node->parent->parent->data._coll == c_externalDeclaration) {
                        clazz = z_global_var;
                    } else {
                        clazz = z_local_var;
                    }
                } else {
                    clazz = z_struct_var;
                }
                auto type = std::dynamic_pointer_cast<type_t>((tmp.rbegin() + 1)->front());
                auto ptr = 0;
                for (auto &ast : asts) {
                    if (AST_IS_OP_N(ast, op_times)) {
                        ptr++;
                    } else {
                        auto new_type = type->clone();
                        new_type->ptr = ptr;
                        add_id(new_type, clazz, ast);
                        ptr = 0;
                    }
                }
            }
                break;
            case c_initDeclarator:
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
                if (node->parent->parent->data._coll == c_functionDefinition) {
                    type_t::ref type;
                    for (auto t = tmp.rbegin() + 1; t != tmp.rend(); t++) {
                        if (!t->empty()) {
                            auto tt = t->front();
                            if (tt->get_base_type() == s_type) {
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
                    symbols[0].insert(std::make_pair(nodes[0]->data._string, func));
                    auto &tmps = tmp.back();
                    std::unordered_set<string_t> ids;
                    auto ptr = 0;
                    for (auto i = 1, j = 0; i < asts.size() && j < tmp.size(); ++i) {
                        auto &pa = asts[i];
                        if (AST_IS_ID(pa)) {
                            const auto &pt = std::dynamic_pointer_cast<type_t>(tmps[j]);
                            pt->ptr = ptr;
                            auto &name = pa->data._string;
                            auto id = std::make_shared<sym_id_t>(pt, name);
                            id->line = pa->line;
                            id->column = pa->column;
                            id->clazz = z_param_var;
                            allocate(id);
                            func->params.push_back(id);
                            if (!ids.insert(name).second) {
                                error(id.get(), "conflict id: " + id->to_string());
                            }
                            {
                                auto f = symbols[0].find(pa->data._string);
                                if (f != symbols[0].end()) {
                                    if (f->second->get_type() == s_function) {
                                        error(id.get(), "conflict id with function: " + id->to_string());
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
                    tmps.push_back(ctx.lock());
                    asts.clear();
#if LOG_TYPE
                    std::cout << "[DEBUG] Func: " << ctx.lock()->to_string() << std::endl;
#endif
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

    void cgen::error(sym_t *sym, const string_t &str) {
        std::stringstream ss;
        ss << "GEN ERROR: " << "[" << sym->line << ":" << sym->column << "] " << str;
        throw cexception(ss.str());
    }

    type_exp_t::ref cgen::to_exp(sym_t::ref s) {
        assert(s->get_base_type() == s_expression);
        return std::dynamic_pointer_cast<type_exp_t>(s);
    }

    void cgen::allocate(sym_id_t::ref id) {
        assert(id->base->get_base_type() == s_type);
        auto type = id->base;
        if (id->clazz == z_global_var) {
            id->addr = data.size();
            auto size = align4(type->size());
            for (auto i = 0; i < size; ++i) {
                data.push_back(0);
            }
        } else if (id->clazz == z_local_var) {
            auto size = align4(type->size());
            auto func = std::dynamic_pointer_cast<sym_func_t>(ctx.lock());
            func->ebp_local += size;
            id->addr = func->ebp_local;
        } else if (id->clazz == z_param_var) {
            auto size = align4(type->size());
            auto func = std::dynamic_pointer_cast<sym_func_t>(ctx.lock());
            id->addr = func->ebp;
            func->ebp += size;
        } else if (id->clazz == z_struct_var) {
            auto _struct = std::dynamic_pointer_cast<sym_struct_t>(ctx.lock());
            _struct->decls.push_back(id);
        } else {
            assert(!"not supported");
        }
    }

    void cgen::add_id(const type_base_t::ref &type, sym_class_t clazz, ast_node *node) {
        assert(AST_IS_ID(node));
        auto new_id = std::make_shared<sym_id_t>(type, node->data._string);
        new_id->line = node->line;
        new_id->column = node->column;
        new_id->clazz = clazz;
        allocate(new_id);
#if LOG_TYPE
        std::cout << "[DEBUG] Id: " << new_id->to_string() << std::endl;
#endif
        if (!symbols.back().insert(std::make_pair(node->data._string, new_id)).second) {
            error(new_id.get(), "conflict id: " + new_id->to_string());
        }
        if (symbols.size() > 1) {
            auto f = symbols[0].find(node->data._string);
            if (f != symbols[0].end()) {
                if (f->second->get_type() == s_function) {
                    error(new_id.get(), "conflict id with function: " + new_id->to_string());
                }
            }
        }
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
                    error("undefined id: " + string_t(node->data._string));
                if (sym->get_type() != s_id)
                    error("required id but got: " + sym->to_string());
                t = std::dynamic_pointer_cast<sym_id_t>(sym)->base->clone();
            }
                break;
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
