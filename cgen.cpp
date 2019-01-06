//
// Project: clibparser
// Created by bajdcc
//

#include <iostream>
#include "cgen.h"
#include "cast.h"
#include "cexception.h"

#define AST_IS_KEYWORD(node) ((node)->flag == ast_keyword)
#define AST_IS_KEYWORD_N(node, k) (AST_IS_KEYWORD(node) && (node)->data._keyword == (k))
#define AST_IS_OP(node) ((node)->flag == ast_operator)
#define AST_IS_OP_N(node, k) (AST_IS_OP(node) && (node)->data._op == (k))

#define LOG_TYPE 1

namespace clib {

    string_t sym_t::to_string() {
        return "[Symbol]";
    }

    type_base_t::type_base_t(lexer_t type) : type(type) {}

    string_t type_base_t::to_string() {
        return LEX_STRING(type);
    }

    type_ptr_t::type_ptr_t(const std::shared_ptr<type_t> &base, int ptr): base(base), ptr(ptr) {}

    string_t type_ptr_t::to_string() {
        std::stringstream ss;
        ss << base->to_string();
        for (int i = 0; i < ptr; ++i) {
            ss << '*';
        }
        return ss.str();
    }

    void cgen::gen(ast_node *node) {
        symbols.clear();
        tmp.clear();
        tmp.emplace_back();
        ast.clear();
        ast.emplace_back();
        gen_rec(node, 0);
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
        tmp.emplace_back();
        ast.emplace_back();
        switch (type) {
            case ast_root: {// 根结点，全局声明
                gen_recursion(node->child, level, rec);
            }
                break;
            case ast_collection: {
                auto children = gen_get_children(node->child);
                gen_coll(children, level + 1, node->data._coll);
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
            (tmp.rbegin() + 1)->push_back(tmp.back().front());
        }
        tmp.pop_back();
        if (!ast.back().empty()) {
            auto &top = ast[ast.size() - 2];
            std::copy(ast.back().begin(), ast.back().end(), std::back_inserter(top));
        }
        ast.pop_back();
    }

    void cgen::gen_coll(const std::vector<ast_node *> &nodes, int level, coll_t t) {
        switch (t) {
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
            case c_structOrUnionSpecifier:
                break;
            case c_structOrUnion:
                break;
            case c_structDeclarationList:
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
                break;
            case c_declarationList:
                break;
        }
        for (auto &node : nodes) {
            gen_rec(node, level);
        }
        auto &asts = ast.back();
        switch (t) {
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
            case c_declarationSpecifiers: {
                std::shared_ptr<type_t> base_type;
                if (AST_IS_KEYWORD_N(asts[0], k_unsigned)) { // unsigned ...
                    if (asts.size() == 1) {
                        asts.erase(asts.begin());
                        base_type = std::make_shared<type_base_t>(l_int);
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
                                error("invalid unsigned * type");
                                break;
                        }
                        asts.erase(asts.begin());
                        asts.erase(asts.begin());
                        base_type = std::make_shared<type_base_t>(type);
                    }
                } else {
                    lexer_t type;
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
                            error("invalid unsigned * type");
                            break;
                    }
                    asts.erase(asts.begin());
                    base_type = std::make_shared<type_base_t>(type);
                }
                if (!asts.empty()) {
                    for (auto &a : asts) {
                        assert(AST_IS_OP_N(a, op_times));
                    }
                    base_type = std::make_shared<type_ptr_t>(base_type, asts.size());
                }
                asts.clear();
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
                break;
            case c_initDeclarator:
                break;
            case c_storageClassSpecifier:
                break;
            case c_typeSpecifier:
                break;
            case c_structOrUnionSpecifier:
                break;
            case c_structOrUnion:
                break;
            case c_structDeclarationList:
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
}
