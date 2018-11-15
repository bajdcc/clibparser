//
// Project: CMiniLang
// Author: bajdcc
//

#include <iomanip>
#include "cexception.h"
#include "cparser.h"
#include "clexer.h"
#include "cast.h"

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
        auto &plus = unit.token(op_plus);
        auto &times = unit.token(op_times);
        program = plus + times | plus;
    }

    void cparser::program() {
        next();
    }

    void cparser::expect(bool flag, const string_t &info) {
        if (!flag) {
            error(info);
        }
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
