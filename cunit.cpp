//
// Project: clibparser
// Created by CC
//

#include <algorithm>
#include <functional>
#include <queue>
#include <iostream>
#include "cunit.h"
#include "cexception.h"

#define SHOW_LABEL 0
#define SHOW_CLOSURE 0

#define IS_SEQ(type) (type == u_sequence)
#define IS_BRANCH(type) (type == u_branch)
#define IS_COLLECTION(type) (type == u_sequence || type == u_branch || type == u_optional)

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
        rule->child = builder->copy(const_cast<unit *>(&u));
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

    unit &unit::operator*() {
        return builder->optional(this);
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

    unit_collection &cunit::optional(unit *a) {
        a = copy(a);
        a->next = a->prev = a;
        return (unit_collection &) (*nodes.alloc<unit_collection>()).set_child(a).set_t(u_optional).init(this);
    }

    unit &cunit::rule(const string_t &s) {
        auto f = rules.find(s);
        if (f == rules.end()) {
            auto &rule = (*nodes.alloc<unit_rule>()).set_s(str(s)).set_t(u_rule).init(this);
            nga_rule r;
            r.id = rules.size();
            r.status = nullptr;
            r.u = to_rule(&rule);
            r.recursive = 0;
            rules.insert(std::make_pair(s, r));
            return *r.u;
        }
        return *f->second.u;
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
            case u_branch:
            case u_optional: {
                auto co = to_collection(node);
                if (node->t == u_branch)
                    os << "( ";
                else if (node->t == u_optional)
                    os << "[ ";
                unit_recursion(co->child, node, os, rec);
                if (node->t == u_branch)
                    os << " )";
                else if (node->t == u_optional)
                    os << " ]";
            }
                break;
            default:
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
        check_nga();
    }

    void cunit::gen_nga() {
        for (auto &rule : rules) {
            current_rule = to_rule(rule.second.u);
            rule.second.status = delete_epsilon(conv_nga(current_rule->child));
        }
        current_rule = nullptr;
    }

    template <class T>
    static std::vector<T *> get_children(T *node) {
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

    bool get_first_set(unit *node, nga_rule &rule) {
        if (node == nullptr)
            return true;
        switch (node->t) {
            case u_none:
                break;
            case u_token_ref: {
                rule.tokensFirstset.insert(to_token(to_ref(node)->child));
                return false;
            }
            case u_rule_ref: {
                rule.rulesFirstset.insert(to_rule(to_ref(node)->child));
                return false;
            }
            case u_sequence: {
                for (auto &u : get_children(to_collection(node)->child)) {
                    if (!get_first_set(u, rule)) {
                        return false;
                    }
                }
                return true;
            }
            case u_branch: {
                auto zero = false;
                for (auto &u : get_children(to_collection(node)->child)) {
                    if (get_first_set(u, rule) && !zero) {
                        zero = true;
                    }
                }
                return zero;
            }
            case u_optional: {
                for (auto &u : get_children(to_collection(node)->child)) {
                    get_first_set(u, rule);
                }
                return true;
            }
            default:
                break;
        }
    }

    void cunit::check_nga() {
        for (auto &rule : rules) {
            nga_rule &r = rule.second;
            if (get_first_set(to_rule(r.u)->child, r)) {
                print(r.u, nullptr, std::cout);
                std::cout << std::endl;
                throw cexception("generate epsilon");
            }
        }
        auto size = rules.size();
        std::vector<nga_rule *> rulesList(size);
        std::vector<std::vector<bool>> dep(size);
        std::unordered_map<unit *, int> ids;
        for (auto &rule : rules) {
            rulesList[rule.second.id] = &rule.second;
            ids.insert(std::make_pair(rule.second.u, rule.second.id));
        }
        for (auto i = 0; i < size; ++i) {
            dep[i].resize(size);
            for (auto &r : rulesList[i]->rulesFirstset) {
                dep[i][ids[r]] = true;
            }
        }
        for (auto i = 0; i < size; ++i) {
            if (dep[i][i]) {
                rulesList[i]->recursive = 1;
                dep[i][i] = false;
            }
        }
        {
            // INDIRECT LEFT RECURSION DETECTION
            std::vector<std::vector<bool>> a(dep), b(dep), r(size);
            for (auto i = 0; i < size; ++i) {
                r[i].resize(size);
            }
            for (auto l = 2; l < size; ++l) {
                for (auto i = 0; i < size; ++i) {
                    for (auto j = 0; j < size; ++j) {
                        r[i][j] = false;
                        for (auto k = 0; k < size; ++k) {
                            r[i][j] = r[i][j] || (a[i][k] && b[k][j]);
                        }
                    }
                }
                for (auto i = 0; i < size; ++i) {
                    if (r[i][i]) {
                        if (rulesList[i]->recursive < 2)
                            rulesList[i]->recursive = l;
                    }
                }
                a = r;
            }
            for (auto i = 0; i < size; ++i) {
                if (rulesList[i]->recursive > 1) {
                    print(rulesList[i]->u, nullptr, std::cout);
                    std::cout << std::endl;
                    throw cexception("indirect left recursion");
                }
            }
        }
        {
            // CALCULATE FIRST SET
            std::vector<bool> visited(size);
            for (auto i = 0; i < size; ++i) {
                auto indep = -1;
                for (auto j = 0; j < size; ++j) {
                    if (!visited[j]) {
                        auto flag = true;
                        for (auto k = 0; k < size; ++k) {
                            if (dep[j][k]) {
                                flag = false;
                                break;
                            }
                        }
                        if (flag) {
                            indep = j;
                        }
                    }
                }
                if (indep == -1) {
                    throw cexception("missing most independent rule");
                }
                for (auto &r : rulesList[indep]->rulesFirstset) {
                    auto &a =  rulesList[indep]->tokensFirstset;
                    auto &b = rulesList[ids[r]]->tokensList;
                    a.insert(b.begin(), b.end());
                }
                {
                    auto &a = rulesList[indep]->tokensList;
                    auto &b = rulesList[indep]->tokensFirstset;
                    a.insert(b.begin(), b.end());
                    auto &c = rulesList[indep]->rulesFirstset;
                    if (c.find(rulesList[indep]->u) != c.end()) {
                        b.insert(a.begin(), a.end());
                    }
                }
                visited[indep] = true;
                for (auto j = 0; j < size; ++j) {
                    dep[j][indep] = false;
                }
            }
            for (auto i = 0; i < size; ++i) {
                if (rulesList[i]->tokensFirstset.empty()) {
                    print(rulesList[i]->u, nullptr, std::cout);
                    std::cout << std::endl;
                    throw cexception("empty first set");
                }
            }
        }
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
            case u_optional: {
                std::vector<nga_edge *> edges;
                nga_recursion(u, edges, rec);
                auto &edge = edges.front();
                return u->builder->connect(edge->begin, edge->end);
            }
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
        auto begin = status();
        auto end = status();
        begin->label = label(node, true);
        end->label = label(node, false);
        auto _enga = connect(begin, end);
        _enga->data = u;
        return _enga;
    }

    nga_edge *cunit::connect(nga_status *a, nga_status *b) {
        auto new_edge = nodes.alloc<nga_edge>();
        new_edge->begin = a;
        new_edge->end = b;
        new_edge->data = nullptr;
        add_edge(a->out, new_edge);
        add_edge(b->in, new_edge);
        return new_edge;
    }

    void *cunit::disconnect(nga_status *status) {
        auto ins = get_children(status->in);
        for (auto &edge : ins) {
            remove_edge(edge->edge->end->in, edge);
            nodes.free(edge);
        }
        auto outs = get_children(status->out);
        for (auto &edge : outs) {
            remove_edge(edge->edge->end->in, edge);
            nodes.free(edge);
        }
        nodes.free(status);
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

    void cunit::remove_edge(nga_edge_list *&list, nga_edge_list *edge) {
        if (list == nullptr) {
            throw cexception("remove from empty edge list");
        } else if (list->next == list) {
            assert(list->edge == edge->edge);
            list = nullptr;
            nodes.free(list);
        } else {
            if (list->edge == edge->edge) {
                list->prev->next = list->next;
                list->next->prev = list->prev;
                list = list->prev;
                nodes.free(edge);
            } else {
                auto node = list->next;
                while (node != list) {
                    if (node->edge == edge->edge) {
                        node->prev->next = node->next;
                        node->next->prev = node->prev;
                        nodes.free(node);
                        return;
                    }
                    node = node->next;
                }
                throw cexception("remove nothing from edge list");
            }
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
            case u_branch:
            case u_optional: {
                auto co = to_collection(node);
                if (node->t == u_branch)
                    os << "( ";
                else if (node->t == u_optional)
                    os << "[ ";
                label_recursion(co->child, node, focused, front, os, rec);
                if (node->t == u_branch)
                    os << " )";
                else if (node->t == u_optional)
                    os << " ]";
            }
                break;
            default:
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

    template<class T>
    static bool has_filter_in_edges(nga_status *status, const T &f) {
        auto node = status->in;
        if (node == nullptr)
            return false;
        auto i = node;
        if (i->next == i) {
            return f(i->edge);
        }
        if (f(i->edge))
            return true;
        i = i->next;
        while (i != node) {
            if (f(i->edge))
                return true;
            i = i->next;
        }
        return false;
    }

    template<class T>
    static std::vector<nga_edge_list *> get_filter_out_edges(nga_status *status, const T &f) {
        std::vector<nga_edge_list *> v;
        auto node = status->out;
        if (node == nullptr)
            return v;
        auto i = node;
        if (i->next == i) {
            if (f(i->edge))
                v.push_back(i);
            return v;
        }
        if (f(i->edge))
            v.push_back(i);
        i = i->next;
        while (i != node) {
            if (f(i->edge))
                v.push_back(i);
            i = i->next;
        }
        return v;
    }

    template<class T>
    std::vector<nga_status *> get_closure(nga_status *status, const T &f) {
        std::vector<nga_status *> v;
        std::queue<nga_status *> queue;
        std::unordered_set<nga_status *> set;
        queue.push(status);
        set.insert(status);
        while (!queue.empty()) {
            auto current = queue.front();
            queue.pop();
            v.push_back(current);
            auto out_ptr = current->out;
            if (out_ptr) {
                auto out = out_ptr->edge->end;
                if (f(out_ptr->edge) && set.find(out) == set.end()) {
                    set.insert(out);
                    queue.push(out);
                }
                if (out_ptr->next != out_ptr) {
                    auto head = out_ptr;
                    out_ptr = out_ptr->next;
                    while (out_ptr != head) {
                        out = out_ptr->edge->end;
                        if (f(out_ptr->edge) && set.find(out) == set.end()) {
                            set.insert(out);
                            queue.push(out);
                        }
                        out_ptr = out_ptr->next;
                    }
                }
            }
        }
        return v;
    }

    nga_status *cunit::delete_epsilon(nga_edge *edge) {
        edge->end->final = true;
        auto nga_status_list = get_closure(edge->begin, [](auto it) { return true; });
#if SHOW_CLOSURE
        for (auto & c : nga_status_list) {
            printf("%s\n", c->label);
        }
#endif
        // TODO: Complete Delete Epsilon
        std::vector<nga_status *> available_status;
        std::vector<const char *> available_labels;
        std::unordered_map<size_t, size_t> available_labels_map;
        available_labels.emplace_back(nga_status_list[0]->label);
        available_labels_map.insert(std::make_pair(std::hash<string_t>{}(string_t(nga_status_list[0]->label)), available_status.size()));
        available_status.push_back(nga_status_list[0]);
        for (auto _status = nga_status_list.begin() + 1; _status != nga_status_list.end(); _status++) {
            auto &status = *_status;
            if (has_filter_in_edges(status, [](auto it) { return it->data != nullptr; })
                && available_labels_map.find(std::hash<string_t>{}(status->label)) ==
                   available_labels_map.end()) {
                available_labels.emplace_back(status->label);
                available_labels_map.insert(std::make_pair(std::hash<string_t>{}(status->label), available_status.size()));
                available_status.push_back(status);
            }
        }
        for (auto &status : available_status) {
            auto epsilon_closure = get_closure(status, [](auto it) { return it->data == nullptr; });
            for (auto &epsilon : epsilon_closure) {
                if (epsilon == status)
                    continue;
                if (epsilon->final)
                    status->final = true;
                auto out_edges = get_filter_out_edges(epsilon, [](auto it) { return it->data != nullptr; });
                for (auto &out_edge: out_edges) {
                    auto idx = available_labels_map.at(std::hash<string_t>{}(out_edge->edge->end->label));
                    connect(status, available_status[idx])->data = out_edge->edge->data;
                }
            }
        }
        for (auto &status : nga_status_list) {
            auto out_edges = get_filter_out_edges(status, [](auto it) { return it->data == nullptr; });
            for (auto &out_edge : out_edges) {
                remove_edge(out_edge->edge->end->in, out_edge);
                remove_edge(status->out, out_edge);
            }
        }
        for (auto &status : nga_status_list) {
            if (std::find(available_status.begin(), available_status.end(), status) == available_status.end()) {
                disconnect(status);
            }
        }
        return edge->begin;
    }

    void print(nga_status *node, std::ostream &os) {
        if (node == nullptr)
            return;
        auto nga_status_list = get_closure(node, [](auto it){ return true; });
        std::unordered_map<nga_status *, size_t> status_map;
        for (int i = 0; i < nga_status_list.size(); ++i) {
            status_map.insert(std::make_pair(nga_status_list[i], i));
        }
        for (auto status : nga_status_list) {
            os << "Status #" << status_map[status];
            if (status->final)
                os << " [FINAL]";
            os << " - " << status->label << std::endl;
            auto outs = get_filter_out_edges(status, [](auto it){ return true; });
            for (auto &out: outs) {
                os << "  To #" << status_map[out->edge->end] << ":  ";
                if (out->edge->data)
                    print(out->edge->data, nullptr, os);
                else
                    os << "EPSILON";
                os << std::endl;
            }
        }
    }

    void cunit::dump(std::ostream &os) {
        os << "==== RULE ====" << std::endl;
        for (auto &k : rules) {
            print(k.second.u, nullptr, os);
            os << std::endl;
        }
        os << "==== NGA  ====" << std::endl;
        for (auto &k : rules) {
            os << "** Rule: " << k.first << std::endl;
            print(k.second.status, os);
            os << std::endl;
        }
    }
};