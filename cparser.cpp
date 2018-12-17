//
// Project: CMiniLang
// Author: bajdcc
//

#include <iomanip>
#include <iostream>
#include <algorithm>
#include "cexception.h"
#include "cparser.h"
#include "clexer.h"
#include "cast.h"
#include "cunit.h"

#define TRACE_PARSING 1

namespace clib {

    cparser::cparser(const string_t &str)
        : lexer(str) {}

    ast_node *cparser::parse() {
        // 清空词法分析结果
        lexer.reset();
        // 清空AST
        ast.reset();
        // 产生式
        gen();
        // 语法分析（递归下降）
        program();
        return ast.get_root();
    }

    ast_node *cparser::root() const {
        return ast.get_root();
    }

    void cparser::next() {
        lexer_t token;
        do {
            token = lexer.next();
            if (token == l_error) {
                auto err = lexer.recent_error();
                printf("[%04d:%03d] %-12s - %s\n",
                       err.line,
                       err.column,
                       ERROR_STRING(err.err).c_str(),
                       err.str.c_str());
            }
        } while (token == l_newline || token == l_space || token == l_error);
#if 0
        if (token != l_end) {
            qDebug("[%04d:%03d] %-12s - %s\n",
                   lexer.get_last_line(),
                   lexer.get_last_column(),
                   LEX_STRING(lexer.get_type()).c_str(),
                   lexer.current().c_str());
        }
#endif
    }

    void cparser::gen() {
        // REFER: antlr/grammars-v4
        // URL: https://github.com/antlr/grammars-v4/blob/master/c/C.g4
#define DEF_KEYWORD(name) auto &_##name##_ = unit.token(k_##name)
        DEF_KEYWORD(auto);
        DEF_KEYWORD(bool);
        DEF_KEYWORD(break);
        DEF_KEYWORD(case);
        DEF_KEYWORD(char);
        DEF_KEYWORD(const);
        DEF_KEYWORD(continue);
        DEF_KEYWORD(default);
        DEF_KEYWORD(do);
        DEF_KEYWORD(double);
        DEF_KEYWORD(else);
        DEF_KEYWORD(enum);
        DEF_KEYWORD(extern);
        DEF_KEYWORD(false);
        DEF_KEYWORD(float);
        DEF_KEYWORD(for);
        DEF_KEYWORD(goto);
        DEF_KEYWORD(if);
        DEF_KEYWORD(int);
        DEF_KEYWORD(long);
        DEF_KEYWORD(register);
        DEF_KEYWORD(return);
        DEF_KEYWORD(short);
        DEF_KEYWORD(signed);
        DEF_KEYWORD(sizeof);
        DEF_KEYWORD(static);
        DEF_KEYWORD(struct);
        DEF_KEYWORD(switch);
        DEF_KEYWORD(true);
        DEF_KEYWORD(typedef);
        DEF_KEYWORD(union);
        DEF_KEYWORD(unsigned);
        DEF_KEYWORD(void);
        DEF_KEYWORD(volatile);
        DEF_KEYWORD(while);
#undef DEF_KEYWORD
#define DEF_OP(name) auto &_##name##_ = unit.token(op_##name)
        DEF_OP(assign);
        DEF_OP(equal);
        DEF_OP(plus);
        DEF_OP(plus_assign);
        DEF_OP(minus);
        DEF_OP(minus_assign);
        DEF_OP(times);
        DEF_OP(times_assign);
        DEF_OP(divide);
        DEF_OP(div_assign);
        DEF_OP(bit_and);
        DEF_OP(and_assign);
        DEF_OP(bit_or);
        DEF_OP(or_assign);
        DEF_OP(bit_xor);
        DEF_OP(xor_assign);
        DEF_OP(mod);
        DEF_OP(mod_assign);
        DEF_OP(less_than);
        DEF_OP(less_than_or_equal);
        DEF_OP(greater_than);
        DEF_OP(greater_than_or_equal);
        DEF_OP(logical_not);
        DEF_OP(not_equal);
        DEF_OP(escape);
        DEF_OP(query);
        DEF_OP(bit_not);
        DEF_OP(lparan);
        DEF_OP(rparan);
        DEF_OP(lbrace);
        DEF_OP(rbrace);
        DEF_OP(lsquare);
        DEF_OP(rsquare);
        DEF_OP(comma);
        DEF_OP(dot);
        DEF_OP(semi);
        DEF_OP(colon);
        DEF_OP(plus_plus);
        DEF_OP(minus_minus);
        DEF_OP(logical_and);
        DEF_OP(logical_or);
        DEF_OP(pointer);
        DEF_OP(left_shift);
        DEF_OP(right_shift);
        DEF_OP(left_shift_assign);
        DEF_OP(right_shift_assign);
        DEF_OP(ellipsis);
#undef DEF_OP
#define DEF_LEX(name, real) auto &real = unit.token(l_##name)
        DEF_LEX(char, Char);
        DEF_LEX(uchar, UnsignedChar);
        DEF_LEX(short, Short);
        DEF_LEX(ushort, UnsignedShort);
        DEF_LEX(int, Integer);
        DEF_LEX(uint, UnsignedInteger);
        DEF_LEX(long, Long);
        DEF_LEX(ulong, UnsignedLong);
        DEF_LEX(float, Float);
        DEF_LEX(double, Double);
        DEF_LEX(identifier, Identifier);
        DEF_LEX(string, String);
        DEF_LEX(comment, Comment);
        DEF_LEX(space, Space);
        DEF_LEX(newline, Newline);
#undef DEF_LEX
#define DEF_RULE(name) auto &name = unit.rule(#name)
        DEF_RULE(program);
        DEF_RULE(primaryExpression);
        DEF_RULE(constant);
        DEF_RULE(postfixExpression);
        DEF_RULE(argumentExpressionList);
        DEF_RULE(unaryExpression);
        DEF_RULE(unaryOperator);
        DEF_RULE(castExpression);
        DEF_RULE(multiplicativeExpression);
        DEF_RULE(additiveExpression);
        DEF_RULE(shiftExpression);
        DEF_RULE(relationalExpression);
        DEF_RULE(equalityExpression);
        DEF_RULE(andExpression);
        DEF_RULE(exclusiveOrExpression);
        DEF_RULE(inclusiveOrExpression);
        DEF_RULE(logicalAndExpression);
        DEF_RULE(logicalOrExpression);
        DEF_RULE(conditionalExpression);
        DEF_RULE(assignmentExpression);
        DEF_RULE(assignmentOperator);
        DEF_RULE(expression);
        DEF_RULE(constantExpression);
        DEF_RULE(declaration);
        DEF_RULE(declarationSpecifiers);
        DEF_RULE(declarationSpecifiers2);
        DEF_RULE(declarationSpecifier);
        DEF_RULE(initDeclaratorList);
        DEF_RULE(initDeclarator);
        DEF_RULE(storageClassSpecifier);
        DEF_RULE(typeSpecifier);
        DEF_RULE(structOrUnionSpecifier);
        DEF_RULE(structOrUnion);
        DEF_RULE(structDeclarationList);
        DEF_RULE(structDeclaration);
        DEF_RULE(specifierQualifierList);
        DEF_RULE(structDeclaratorList);
        DEF_RULE(structDeclarator);
        DEF_RULE(enumSpecifier);
        DEF_RULE(enumeratorList);
        DEF_RULE(enumerator);
        DEF_RULE(enumerationConstant);
        DEF_RULE(typeQualifier);
        DEF_RULE(declarator);
        DEF_RULE(directDeclarator);
        DEF_RULE(pointer);
        DEF_RULE(typeQualifierList);
        DEF_RULE(parameterTypeList);
        DEF_RULE(parameterList);
        DEF_RULE(parameterDeclaration);
        DEF_RULE(identifierList);
        DEF_RULE(typeName);
        DEF_RULE(abstractDeclarator);
        DEF_RULE(directAbstractDeclarator);
        DEF_RULE(typedefName);
        DEF_RULE(initializer);
        DEF_RULE(initializerList);
        DEF_RULE(designation);
        DEF_RULE(designatorList);
        DEF_RULE(designator);
        DEF_RULE(statement);
        DEF_RULE(labeledStatement);
        DEF_RULE(compoundStatement);
        DEF_RULE(blockItemList);
        DEF_RULE(blockItem);
        DEF_RULE(expressionStatement);
        DEF_RULE(selectionStatement);
        DEF_RULE(iterationStatement);
        DEF_RULE(forCondition);
        DEF_RULE(forDeclaration);
        DEF_RULE(forExpression);
        DEF_RULE(jumpStatement);
        DEF_RULE(compilationUnit);
        DEF_RULE(translationUnit);
        DEF_RULE(externalDeclaration);
        DEF_RULE(functionDefinition);
        DEF_RULE(declarationList);
#undef DEF_RULE
        program = compilationUnit;
        primaryExpression = Identifier
                            | constant
                            | String
                            | _lparan_ + expression + _rparan_;
        constant = Char | UnsignedChar | Short | UnsignedShort | Integer | UnsignedInteger |
                   Long | UnsignedLong | Float | Double;
        postfixExpression = primaryExpression
                            | postfixExpression + (_lsquare_ + expression + _rsquare_
                                                   | _lparan_ + *argumentExpressionList + _rparan_
                                                   | _dot_ + Identifier
                                                   | _pointer_ + Identifier
                                                   | _plus_plus_
                                                   | _minus_minus_)
                            | _lparan_ + typeName + _rparan_ + _lbrace_ + initializerList + *_comma_ + _rbrace_;
        argumentExpressionList = *(argumentExpressionList + _comma_) + assignmentExpression;
        unaryExpression
            = postfixExpression
              | (_plus_plus_ | _minus_minus_) + unaryExpression
              | unaryOperator + castExpression
              | _sizeof_ + (unaryExpression | _lparan_ + typeName + _rparan_);
        unaryOperator = _bit_and_ | _times_ | _plus_ | _minus_ | _bit_not_ | _logical_not_;
        castExpression = _lparan_ + typeName + _rparan_ + castExpression | unaryExpression;
        multiplicativeExpression = castExpression | multiplicativeExpression +
                                                    (_times_ + castExpression | _divide_ + castExpression |
                                                     _mod_ + castExpression);
        additiveExpression = *(additiveExpression + (_plus_ | _minus_)) + multiplicativeExpression;
        shiftExpression = *(shiftExpression + (_left_shift_ | _right_shift_)) + additiveExpression;
        relationalExpression = *(relationalExpression +
                                 (_less_than_ | _greater_than_ | _less_than_or_equal_ | _greater_than_or_equal_)) +
                               shiftExpression;
        equalityExpression = *(equalityExpression + (_equal_ | _not_equal_)) + relationalExpression;
        andExpression = *(andExpression + _bit_and_) + equalityExpression;
        exclusiveOrExpression = *(exclusiveOrExpression + _bit_xor_) + andExpression;
        inclusiveOrExpression = *(inclusiveOrExpression + _bit_or_) + exclusiveOrExpression;
        logicalAndExpression = *(logicalAndExpression + _logical_and_) + inclusiveOrExpression;
        logicalOrExpression = *(logicalOrExpression + _logical_or_) + logicalAndExpression;
        conditionalExpression = logicalOrExpression + *(_query_ + expression + _assign_ + conditionalExpression);
        assignmentExpression = conditionalExpression | unaryExpression + assignmentOperator + assignmentExpression;
        assignmentOperator = _assign_ | _times_assign_ | _div_assign_ | _mod_assign_ |
                             _plus_assign_ | _minus_assign_ | _left_shift_assign_ | _right_shift_assign_;
        expression = *(expression + _comma_) + assignmentExpression;
        constantExpression = conditionalExpression;
        declaration = declarationSpecifiers + *initDeclaratorList + _semi_;
        declarationSpecifiers = declarationSpecifier + *declarationSpecifiers;
        declarationSpecifiers2 = declarationSpecifier + *declarationSpecifiers;
        declarationSpecifier
            = storageClassSpecifier
              | typeSpecifier
              | typeQualifier;
        initDeclaratorList = *(initDeclaratorList + _comma_) + initDeclarator;
        initDeclarator = declarator + *(_assign_ + initializer);
        storageClassSpecifier = _typedef_ | _extern_ | _static_ | _auto_ | _register_;
        typeSpecifier = _void_ | _char_ | _short_ | _int_ | _long_ | _float_ | _double_ | _signed_ | _unsigned_ | _bool_
                        | structOrUnionSpecifier
                        | enumSpecifier
                        | typedefName
                        | typeSpecifier + pointer;
        structOrUnionSpecifier =
            structOrUnion + (Identifier | (*Identifier + _lbrace_ + structDeclarationList + _rbrace_));
        structOrUnion = _struct_ | _union_;
        structDeclarationList = *structDeclarationList + structDeclaration;
        structDeclaration = specifierQualifierList + *structDeclaratorList + _semi_;
        specifierQualifierList = (typeSpecifier | typeQualifier) + *specifierQualifierList;
        structDeclaratorList = *(structDeclaratorList + _comma_) + structDeclarator;
        structDeclarator = declarator | *declarator + _assign_ + constantExpression;
        enumSpecifier = _enum_ + ((*Identifier + _lbrace_ + enumeratorList + *_comma_ + _lbrace_) | Identifier);
        enumeratorList = *(enumeratorList + _comma_) + enumerator;
        enumerator = enumerationConstant + *(_assign_ + constantExpression);
        enumerationConstant = Identifier;
        typeQualifier = _const_ | _volatile_;
        declarator = *pointer + directDeclarator;
        directDeclarator
            = Identifier
              | _lparan_ + declarator + _rparan_
              | directDeclarator + (_lsquare_ + (*typeQualifierList + (*assignmentExpression | _times_) + _rsquare_
                                                 | _lparan_ + (parameterTypeList | *identifierList) + _rparan_));
        pointer = (_times_ | _bit_xor_) + (*typeQualifierList + *pointer);
        typeQualifierList = *typeQualifierList + typeQualifier;
        parameterTypeList = parameterList + _comma_ + _ellipsis_;
        parameterList = *(parameterList + _comma_) + parameterDeclaration;
        parameterDeclaration = declarationSpecifiers + declarator
                               | declarationSpecifiers2 + *abstractDeclarator;
        identifierList = *(identifierList + _comma_) + Identifier;
        typeName = specifierQualifierList + *abstractDeclarator;
        abstractDeclarator = pointer
                             | *pointer + directAbstractDeclarator;
        directAbstractDeclarator = _lparan_ + abstractDeclarator + _rparan_
                                   | *directAbstractDeclarator +
                                     (_lsquare_ + (*typeQualifierList + (*assignmentExpression | _times_) + _rsquare_)
                                      | _lparan_ + *parameterTypeList + _rparan_);
        typedefName = Identifier;
        initializer = assignmentExpression | _lbrace_ + initializerList + *_comma_ + _rbrace_;
        initializerList = *(initializerList + _comma_) + *designation + initializer;
        designation = designatorList + _assign_;
        designatorList = *designatorList + designator;
        designator = _lsquare_ + constantExpression + _rsquare_
                     | _dot_ + Identifier;
        statement
            = labeledStatement
              | compoundStatement
              | expressionStatement
              | selectionStatement
              | iterationStatement
              | jumpStatement;
        labeledStatement = (Identifier
                            | _case_ + constantExpression
                            | _default_) + _assign_ + statement;
        compoundStatement = _lbrace_ + *blockItemList + _rbrace_;
        blockItemList = *blockItemList + blockItem;
        blockItem = statement | declaration;
        expressionStatement = *expression + _semi_;
        selectionStatement
            = _if_ + _lparan_ + expression + _rparan_ + statement + *(_else_ + statement)
              | _switch_ + _lparan_ + expression + _rparan_ + statement;
        iterationStatement
            = _while_ + _lparan_ + expression + _rparan_ + statement
              | _do_ + statement + _while_ + _lparan_ + expression + _rparan_ + _semi_
              | _for_ + _lparan_ + forCondition + _rparan_ + statement;
        forCondition = (forDeclaration | *expression) + _semi_ + *forExpression + _semi_ + *forExpression;
        forDeclaration = declarationSpecifiers + *initDeclaratorList;
        forExpression = *(forExpression + _comma_) + assignmentExpression;
        jumpStatement = _goto_ +
                        Identifier + _semi_
                        | _continue_ + _semi_
                        | _break_ + _semi_
                        | _return_ + *expression + _semi_;
        compilationUnit = translationUnit + *compilationUnit;
        translationUnit = *translationUnit + externalDeclaration;
        externalDeclaration = functionDefinition | declaration | _semi_;
        functionDefinition = *declarationSpecifiers + declarator + *declarationList + compoundStatement;
        declarationList = *declarationList + declaration;
        unit.gen(&program);
        //unit.dump(std::cout);
    }

    void cparser::program() {
        next();
        state_stack.clear();
        ast_stack.clear();
        state_stack.push_back(0);
        ast_stack.push_back(ast.get_root());
        auto &pdas = unit.get_pda();
        std::vector<int> trans_ids;
        auto state = 0;
        for (;;) {
            auto current_state = pdas[state];
            if (lexer.is_type(l_end)) {
                if (current_state.final) {
                    if (state_stack.empty())
                        break;
                } else {
                    std::cout << current_state.label << std::endl;
                    throw cexception("parsing unexpected EOF");
                }
            }
            auto &trans = current_state.trans;
            trans_ids.clear();
            for (auto i = 0; i < trans.size(); ++i) {
                auto &cs = trans[i];
                if (valid_trans(cs))
                    trans_ids.push_back(i | pda_edge_priority(cs.type) << 16);
            }
            if (trans_ids.empty()) {
                std::cout << current_state.label << std::endl;
                throw cexception("parsing error");
            }
            std::sort(trans_ids.begin(), trans_ids.end());
            auto trans_id = trans_ids.front() & ((1 << 16) - 1);
            auto &t = trans[trans_id];
            if (t.type == e_finish) {
                if (!lexer.is_type(l_end)) {
                    std::cout << current_state.label << std::endl;
                    throw cexception("parsing redundant code");
                }
            }
            auto jump = trans[trans_id].jump;
#if TRACE_PARSING
            printf("State: %3d => To: %3d   -- Action: %-10s -- Rule: %s\n",
                   state, jump, pda_edge_str(t.type).c_str(), current_state.label.c_str());
#endif
            do_trans(trans[trans_id]);
            state = jump;
        }
    }

    ast_node *cparser::terminal() {
        if (lexer.is_type(l_end)) { // 结尾
            error("unexpected token EOF of expression");
        }
        if (lexer.is_type(l_operator)) {
            auto node = ast.new_node(ast_operator);
            node->data._op = lexer.get_operator();
            match_operator(node->data._op);
            return node;
        }
        if (lexer.is_type(l_keyword)) {
            auto node = ast.new_node(ast_keyword);
            node->data._keyword = lexer.get_keyword();
            match_keyword(node->data._keyword);
            return node;
        }
        if (lexer.is_type(l_identifier)) {
            auto node = ast.new_node(ast_literal);
            ast.set_str(node, lexer.get_identifier());
            match_type(l_identifier);
            return node;
        }
        if (lexer.is_number()) {
            ast_node *node = nullptr;
            auto type = lexer.get_type();
            switch (type) {
#define DEFINE_NODE_INT(t) \
            case l_##t: \
                node = ast.new_node(ast_##t); \
                node->data._##t = lexer.get_##t(); \
                break;
                DEFINE_NODE_INT(char)
                DEFINE_NODE_INT(uchar)
                DEFINE_NODE_INT(short)
                DEFINE_NODE_INT(ushort)
                DEFINE_NODE_INT(int)
                DEFINE_NODE_INT(uint)
                DEFINE_NODE_INT(long)
                DEFINE_NODE_INT(ulong)
                DEFINE_NODE_INT(float)
                DEFINE_NODE_INT(double)
#undef DEFINE_NODE_INT
                default:
                    error("invalid number");
                    break;
            }
            match_number();
            return node;
        }
        if (lexer.is_type(l_string)) {
            std::stringstream ss;
            ss << lexer.get_string();
#if 0
            printf("[%04d:%03d] String> %04X '%s'\n", clexer.get_line(), clexer.get_column(), idx, clexer.get_string().c_str());
#endif
            match_type(l_string);

            while (lexer.is_type(l_string)) {
                ss << lexer.get_string();
#if 0
                printf("[%04d:%03d] String> %04X '%s'\n", clexer.get_line(), clexer.get_column(), idx, clexer.get_string().c_str());
#endif
                match_type(l_string);
            }
            auto node = ast.new_node(ast_string);
            ast.set_str(node, ss.str());
            return node;
        }
        error("invalid type");
        return nullptr;
    }

    bool cparser::valid_trans(const pda_trans &trans) const {
        auto &la = trans.LA;
        if (!la.empty()) {
            auto success = false;
            for (auto &_la : la) {
                if (LA(_la)) {
                    success = true;
                    break;
                }
            }
            if (!success)
                return false;
        }
        switch (trans.type) {
            case e_shift:
                break;
            case e_move:
                break;
            case e_left_recursion:
                break;
            case e_reduce: {
                assert(state_stack.size() >= 2);
                if (trans.status != *(state_stack.rbegin() + 1))
                    return false;
            }
                break;
            case e_finish:
                break;
            default:
                break;
        }
        return true;
    }

    void cparser::do_trans(const pda_trans &trans) {
        switch (trans.type) {
            case e_shift: {
                state_stack.push_back(trans.jump);
                ast_stack.push_back(ast.new_node(ast_collection));
            }
                break;
            case e_move: {
                cast::set_child(ast_stack.back(), terminal());
            }
                break;
            case e_left_recursion:
                break;
            case e_reduce: {
                auto new_ast = ast_stack.back();
                state_stack.pop_back();
                ast_stack.pop_back();
                if (cast::children_size(new_ast) > 1) {
                    cast::set_child(ast_stack.back(), new_ast);
                } else {
                    auto child = new_ast->child;
                    ast.remove(new_ast);
                    cast::set_child(ast_stack.back(), child);
                }
            }
                break;
            case e_finish:
                state_stack.pop_back();
                break;
        }
    }

    bool cparser::LA(struct unit *u) const {
        if (u->t != u_token)
            return false;
        auto token = to_token(u);
        if (token->type == l_keyword)
            return lexer.is_keyword(token->value.keyword);
        if (token->type == l_operator)
            return lexer.is_operator(token->value.op);
        return lexer.is_type(token->type);
    }

    void cparser::expect(bool flag, const string_t &info) {
        if (!flag) {
            error(info);
        }
    }

    void cparser::match_keyword(keyword_t type) {
        expect(lexer.is_keyword(type), string_t("expect keyword ") + KEYWORD_STRING(type));
        next();
    }

    void cparser::match_operator(operator_t type) {
        expect(lexer.is_operator(type), string_t("expect operator " + OPERATOR_STRING(type)));
        next();
    }

    void cparser::match_type(lexer_t type) {
        expect(lexer.is_type(type), string_t("expect type " + LEX_STRING(type)));
        next();
    }

    void cparser::match_number() {
        expect(lexer.is_number(), "expect number");
        next();
    }

    void cparser::match_integer() {
        expect(lexer.is_integer(), "expect integer");
        next();
    }

    void cparser::error(const string_t &info) {
        std::stringstream ss;
        ss << '[' << std::setfill('0') << std::setw(4) << lexer.get_line();
        ss << ':' << std::setfill('0') << std::setw(3) << lexer.get_column();
        ss << ']' << " PARSER ERROR: " << info;
        throw cexception(ss.str());
    }
}
