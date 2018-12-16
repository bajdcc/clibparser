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
        auto &program = unit.rule("root");
        auto &exp0 = unit.rule("exp0");
        auto &exp1 = unit.rule("exp1");
        auto &exp2 = unit.rule("exp2");
        auto &plus = unit.token(op_plus);
        auto &minus = unit.token(op_minus);
        auto &times = unit.token(op_times);
        auto &divide = unit.token(op_divide);
        auto &integer = unit.token(l_int);
        program = exp0;
        exp0 = *(exp0 + (plus | minus)) + exp1;
        exp1 = *(exp1 + (times | divide)) + exp2;
        exp2 = integer;
        unit.gen(&program);
        unit.dump(std::cout);
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
