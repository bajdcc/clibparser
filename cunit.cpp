//
// Project: clibparser
// Created by CC
//

#include "cunit.h"

#define IS_SEQ(type) (type == u_sequence)
#define IS_BRANCH(type) (type == u_branch)
#define IS_COLLECTION(type) (type == u_sequence || type == u_branch)

namespace clib {

    unit_rule *to_rule(unit *u) {
        assert(u->t == u_rule);
        return (unit_rule *) u;
    }

    unit_token *to_token(unit *u) {
        assert(u->t == u_token);
        return (unit_token *) u;
    }

    unit_collection *to_collection(unit *u) {
        assert(IS_COLLECTION(u->t));
        return (unit_collection *) u;
    }

    unit_collection *to_ref(unit *u) {
        assert(u->t == u_token_ref);
        return (unit_collection *) u;
    }

    unit &unit::operator=(const unit &u) {
        auto rule = to_rule(this);
        rule->child = const_cast<unit *>(&u);
        return *this;
    }

    unit &unit::operator+(const unit &u) {
        if ((!IS_COLLECTION(t) && !IS_COLLECTION(u.t)) ||
            (IS_BRANCH(t) && IS_BRANCH(u.t))) {
            return builder->collection(this, const_cast<unit *>(&u), u_sequence);
        } else if (IS_SEQ(t) && IS_SEQ(u.t)) {
            return builder->merge(this, const_cast<unit *>(&u));
        } else if (IS_SEQ(t)) {
            return builder->append(this, const_cast<unit *>(&u));
        } else if (IS_SEQ(u.t)) {
            return builder->append(const_cast<unit *>(&u), this);
        } else {
            return builder->collection(this, const_cast<unit *>(&u), u_sequence);
        }
    }

    unit &unit::operator|(const unit &u) {
        if ((!IS_COLLECTION(t) && !IS_COLLECTION(u.t)) ||
            (IS_SEQ(t) && IS_SEQ(u.t))) {
            return builder->collection(this, const_cast<unit *>(&u), u_branch);
        } else if (IS_BRANCH(t) && IS_BRANCH(u.t)) {
            return builder->merge(this, const_cast<unit *>(&u));
        } else if (IS_BRANCH(t)) {
            return builder->append(this, const_cast<unit *>(&u));
        } else if (IS_BRANCH(u.t)) {
            return builder->append(this, const_cast<unit *>(&u));
        } else {
            return builder->collection(this, const_cast<unit *>(&u), u_branch);
        }
    }

    unit &unit::init(unit_builder *builder) {
        next = prev = nullptr;
        this->builder = builder;
        return *this;
    }

    unit &unit::set_t(unit_t type) {
        this->t = type;
        return *this;
    }

    unit_token &unit_token::set_type(lexer_t type) {
        this->type = type;
        return *this;
    }

    unit_token &unit_token::set_op(operator_t op) {
        value.op = op;
        return *this;
    }

    unit_token &unit_token::set_keyword(keyword_t keyword) {
        value.keyword = keyword;
        return *this;
    }

    unit_collection &unit_collection::set_child(unit *node) {
        child = node;
        return *this;
    }

    unit_rule &unit_rule::set_s(const char *str) {
        s = str;
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
        return (*nodes.alloc<unit_token>()).set_type(type).set_t(u_token).init(this);
    }

    unit &cunit::token(const operator_t &op) {
        return (*nodes.alloc<unit_token>()).set_type(l_operator).set_op(op).set_t(u_token).init(this);
    }

    unit &cunit::token(const keyword_t &keyword) {
        return (*nodes.alloc<unit_token>()).set_type(l_keyword).set_keyword(keyword).set_t(u_token).init(this);
    }

    unit *cunit::copy(unit *u) {
        if (u->t == u_token) { // copy token unit
            return &(*nodes.alloc<unit_collection>()).set_child(u).set_t(u_token_ref).init(this);;
        }
        return u;
    }

    unit_collection &cunit::append(unit *collection, unit *child) {
        auto node = to_collection(collection);
        child = copy(child);
        if (node->child == nullptr) { // 没有孩子
            node->child = child;
            child->prev = child->next = child;
        } else { // 有孩子，添加到末尾
            child->prev = node->child->prev;
            child->next = node->child;
            node->child->prev->next = child;
            node->child->prev = child;
        }
        return *node;
    }

    unit_collection &cunit::merge(unit *a, unit *b) {
        auto nodeA = to_collection(a)->child;
        auto nodeB = to_collection(b)->child;
        nodeA->prev->next = nodeB;
        nodeB->prev->next = nodeA;
        std::swap(nodeA->prev, nodeB->prev);
        nodes.free(b);
        return *to_collection(a);
    }

    unit_collection &cunit::collection(unit *a, unit *b, unit_t type) {
        a = copy(a);
        b = copy(b);
        a->next = a->prev = b;
        b->next = b->prev = a;
        return (unit_collection &) (*nodes.alloc<unit_collection>()).set_child(a).set_t(type).init(this);
    }

    unit &cunit::rule(const string_t &s) {
        auto f = rules.find(s);
        if (f == rules.end()) {
            auto &rule = (*nodes.alloc<unit_rule>()).set_s(str(s)).set_t(u_rule).init(this);
            return *rules.insert(std::make_pair(s, &rule)).first->second;
        }
        return *f->second;
    }

    template<class T>
    static void unit_recursion(unit *node, unit *parent, std::ostream &os, T f) {
        if (node == nullptr)
            return;
        auto i = node;
        if (i->next == i) {
            f(i, parent, os);
            return;
        }
        f(i, parent, os);
        i = i->next;
        while (i != node) {
            f(i, parent, os);
            i = i->next;
        }
    }

    void print(unit *node, unit *parent, std::ostream &os) {
        if (node == nullptr)
            return;
        auto rec = [&](auto n, auto p, auto &os) { print(n, p, os); };
        auto type = node->t;
        switch (type) {
            case u_none:
                break;
            case u_token: {
                auto token = to_token(node);
                if (token->type == l_keyword) {
                    os << KEYWORD_STRING(token->value.keyword);
                } else if (token->type == l_operator) {
                    os << "'" << OP_STRING(token->value.op) << "'";
                } else {
                    os << "#" << LEX_STRING(token->type) << "#";
                }
            }
                break;
            case u_token_ref: {
                rec(to_ref(node)->child, node, os);
            }
                break;
            case u_rule: {
                auto rule = to_rule(node);
                os << rule->s << " => ";
                rec(rule->child, node, os);
            }
                break;
            case u_sequence:
            case u_branch: {
                auto co = to_collection(node);
                unit_recursion(co->child, node, os, rec);
            }
                break;
            case u_optional:
                break;
        }
        if (parent) {
            if (IS_COLLECTION(parent->t)) {
                auto p = to_collection(parent);
                if (node->next != p->child)
                    os << (IS_SEQ(p->t) ? " " : " | ");
            }
        }
    }

    void cunit::dump(std::ostream &os) {
        os << "==== RULE ====" << std::endl;
        for (auto &k : rules) {
            print(k.second, nullptr, os);
            os << std::endl;
        }
    }
};