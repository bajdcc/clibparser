//
// Project: clibparser
// Created by CC
//

#include "cunit.h"

namespace clib {

    unit_rule *to_rule(unit *u) {
        assert(u->type == u_rule);
        return (unit_rule*) u;
    }

    unit &unit::operator=(const unit &u) {
        auto rule = to_rule(this);
        rule->child = const_cast<unit*>(&u);
        return *this;
    }

    unit &unit::operator+(const unit &u) {
        if (type == u_sequence) {
            return *this;
        } else {
            return *this;
        }
    }

    unit &unit::operator|(const unit &u) {
        if (type == u_sequence) {
            return *this;
        } else {
            return *this;
        }
    }

    unit &unit_token::operator,(lexer_t type) {
        this->type = type;
        return *this;
    }

    unit &unit_token::operator,(operator_t op) {
        value.op = op;
        return *this;
    }

    unit &unit_token::operator,(keyword_t keyword) {
        value.keyword = keyword;
        return *this;
    }

    const char *cunit::str(const string_t &s) {
        auto f = strings.find(s);
        if (f == strings.end()) {
            return strings.insert(s).first->c_str();
        }
        return f->c_str();
    }

    unit &cunit::token(const lexer_t &type) {
        return *nodes.alloc<unit_token>(), type;
    }

    unit &cunit::token(const operator_t &op) {
        return *nodes.alloc<unit_token>(), op;
    }

    unit &cunit::token(const keyword_t &keyword) {
        return *nodes.alloc<unit_token>(), keyword;
    }

    unit &cunit::rule(const string_t &s) {
        auto f = rules.find(s);
        if (f == rules.end()) {
            auto rule = nodes.alloc<unit_rule>();
            rule->s = str(s);
            return *rules.insert(std::make_pair(s, rule)).first->second;
        }
        return *f->second;
    }
};