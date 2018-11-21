//
// Project: clibparser
// Created by CC
//

#include <functional>
#include "cunit.h"

#define SHOW_LABEL 0

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
        assert(u->t == u_token_ref || u->t == u_rule_ref);
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
        if (u->t == u_rule) { // copy rule unit
            return &(*nodes.alloc<unit_collection>()).set_child(u).set_t(u_rule_ref).init(this);;
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
            case u_rule_ref: {
                os << to_rule(to_ref(node)->child)->s;
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

    void cunit::gen(const unit_rule &sym) {
        gen_nga();
    }

    void cunit::dump(std::ostream &os) {
        os << "==== RULE ====" << std::endl;
        for (auto &k : rules) {
            print(k.second, nullptr, os);
            os << std::endl;
        }
    }

    void cunit::gen_nga() {
        if (!ngas.empty())
            return;
        for (auto &rule : rules) {
            current_rule = to_rule(rule.second);
            ngas.insert(std::make_pair(rule.first, conv_nga(current_rule->child)));
        }
        current_rule = nullptr;
    }

    template<class T>
    static void nga_recursion(unit *u, std::vector<nga_edge *> &v, T f) {
        auto node = to_collection(u)->child;
        if (node == nullptr)
            return;
        auto i = node;
        if (i->next == i) {
            v.push_back(f(i));
            return;
        }
        v.push_back(f(i));
        i = i->next;
        while (i != node) {
            v.push_back(f(i));
            i = i->next;
        }
    }

    nga_edge *cunit::conv_nga(unit *u) {
        if (u == nullptr)
            return nullptr;
        auto rec = [&](auto u) { return conv_nga(u); };
        switch (u->t) {
            case u_token_ref:
            case u_rule_ref:
                return u->builder->enga(u, u);
            case u_sequence: {
                auto enga = u->builder->enga(u, false);
                enga->data = u;
                std::vector<nga_edge *> edges;
                nga_recursion(u, edges, rec);
                for (auto &edge : edges) {
                    if (enga->begin != nullptr) {
                        u->builder->connect(enga->end, edge->begin);
                        enga->end = edge->end;
                    } else {
                        enga->begin = edge->begin;
                        enga->end = edge->end;
                    }
                }
                return enga;
            }
            case u_branch: {
                auto enga = u->builder->enga(u, true);
                enga->data = u;
                std::vector<nga_edge *> edges;
                nga_recursion(u, edges, rec);
                for (auto &edge : edges) {
                    u->builder->connect(enga->begin, edge->begin);
                    u->builder->connect(edge->end, enga->end);
                }
                return enga;
            }
            case u_optional:
                break;
            default:
                break;
        }
        assert(!"not supported");
        return nullptr;
    }

    nga_edge *cunit::enga(unit *node, bool init) {
        auto _enga = nodes.alloc<nga_edge>();
        _enga->data = nullptr;
        if (init) {
            _enga->begin = status();
            _enga->end = status();
            _enga->begin->label = label(node, true);
            _enga->end->label = label(node, false);
        } else {
            _enga->begin = _enga->end = nullptr;
        }
        return _enga;
    }

    nga_edge *cunit::enga(unit *node, unit *u) {
        auto _enga = enga(node, true);
        connect(_enga->begin, _enga->end);
        _enga->data = u;
        return _enga;
    }

    nga_edge *cunit::connect(nga_status *a, nga_status *b) {
        auto new_edge = nodes.alloc<nga_edge>();
        new_edge->begin = a;
        new_edge->end = b;
        add_edge(a->out, new_edge);
        add_edge(b->in, new_edge);
        return new_edge;
    }

    nga_status *cunit::status() {
        auto _status = nodes.alloc<nga_status>();
        _status->final = false;
        _status->label = nullptr;
        _status->in = _status->out = nullptr;
        return _status;
    }

    void cunit::add_edge(nga_edge_list *&list, nga_edge *edge) {
        if (list == nullptr) {
            list = nodes.alloc<nga_edge_list>();
            list->edge = edge;
            list->prev = list->next = list;
        } else {
            auto new_edge = nodes.alloc<nga_edge_list>();
            new_edge->edge = edge;
            new_edge->prev = list->prev;
            new_edge->next = list;
            list->prev->next = new_edge;
            list->prev = new_edge;
        }
    }

    const char *cunit::label(unit *focused, bool front) {
        std::stringstream ss;
        label(current_rule, nullptr, focused, front, ss);
#if SHOW_LABEL
        printf("%s\n", ss.str().c_str());
#endif
        labels.emplace_back(ss.str());
        return labels.back().c_str();
    }

    template<class T>
    static void
    label_recursion(unit *node, unit *parent, unit *focused, bool front, std::ostream &os, T f) {
        if (node == nullptr)
            return;
        auto i = node;
        if (i->next == i) {
            f(i, parent, focused, front, os);
            return;
        }
        f(i, parent, focused, front, os);
        i = i->next;
        while (i != node) {
            f(i, parent, focused, front, os);
            i = i->next;
        }
    }

    void cunit::label(unit *node, unit *parent, unit *focused, bool front, std::ostream &os) {
        if (node == nullptr)
            return;
        auto rec = [this](auto _1, auto _2, auto _3, auto _4, auto &_5) {
            label(_1, _2, _3, _4, _5);
        };
        if (front && node == focused) {
            os << "@ ";
        }
        switch (node->t) {
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
                rec(to_ref(node)->child, node, focused, front, os);
            }
                break;
            case u_rule: {
                auto _rule = to_rule(node);
                os << _rule->s << " => ";
                rec(_rule->child, node, focused, front, os);
            }
                break;
            case u_rule_ref: {
                os << to_rule(to_ref(node)->child)->s;
            }
                break;
            case u_sequence:
            case u_branch: {
                auto co = to_collection(node);
                label_recursion(co->child, node, focused, front, os, rec);
            }
                break;
            case u_optional:
                break;
        }
        if (!front && node == focused) {
            os << " @";
        }
        if (parent) {
            if (IS_COLLECTION(parent->t)) {
                auto p = to_collection(parent);
                if (node->next != p->child)
                    os << (IS_SEQ(p->t) ? " " : " | ");
            }
        }
    }
};