//
// Project: clibparser
// Created by CC
//

#ifndef CLIBPARSER_CUNIT_H
#define CLIBPARSER_CUNIT_H

#include <string>
#include <unordered_set>
#include <vector>
#include <map>
#include "memory.h"

#define UNIT_NODE_MEM (2 * 1024)

namespace clib {

    enum unit_t {
        u_none,
        u_token,
        u_token_ref,
        u_rule,
        u_rule_ref,
        u_sequence,
        u_branch,
        u_optional,
    };

    class unit_builder;

    struct unit {
        unit_t t;
        unit *next;
        unit *prev;
        unit_builder *builder;

        unit &operator=(const unit &u);
        unit &operator+(const unit &u);
        unit &operator|(const unit &u);
        unit &operator*();
        unit &init(unit_builder *builder);
        unit &set_t(unit_t type);
    };

    struct unit_token : public unit {
        lexer_t type;
        union {
            operator_t op;
            keyword_t keyword;
        } value;

        unit_token &set_type(lexer_t type);
        unit_token &set_op(operator_t op);
        unit_token &set_keyword(keyword_t keyword);
    };

    struct unit_collection : public unit {
        unit *child;

        unit_collection &set_child(unit *node);
    };

    struct unit_rule : public unit_collection {
        const char *s;

        unit_rule &set_s(const char *str);
    };

    struct nga_edge;
    struct nga_edge_list;

    struct nga_status {
        const char *label;
        bool final;
        nga_edge_list *in, *out;
    };

    struct nga_edge {
        nga_status *begin, *end;
        unit *data;
    };

    struct nga_edge_list {
        nga_edge_list *prev, *next;
        nga_edge *edge;
    };

    class unit_builder {
    public:
        virtual unit_collection &append(unit *collection, unit *child) = 0;
        virtual unit_collection &merge(unit *a, unit *b) = 0;
        virtual unit_collection &collection(unit *a, unit *b, unit_t type) = 0;
        virtual unit_collection &optional(unit *a) = 0;
        virtual unit *copy(unit *u) = 0;

        virtual nga_edge *enga(unit *node, bool init) = 0;
        virtual nga_edge *enga(unit *node, unit *u) = 0;
        virtual nga_edge *connect(nga_status *a, nga_status *b) = 0;
    };

    struct nga_rule {
        unit *u;
        nga_status *status;
        std::unordered_set<unit_token *> tokens;
        std::unordered_set<unit_rule *> rules;
    };

    // 文法表达式
    class cunit : public unit_builder {
    public:
        cunit() = default;
        ~cunit() = default;

        cunit(const cunit &) = delete;
        cunit &operator=(const cunit &) = delete;

        unit &token(const lexer_t &type);
        unit &token(const operator_t &op);
        unit &token(const keyword_t &keyword);
        unit &rule(const string_t &s);

        unit_collection &append(unit *collection, unit *child) override;
        unit_collection &merge(unit *a, unit *b) override;
        unit_collection &collection(unit *a, unit *b, unit_t type) override;
        unit_collection &optional(unit *a) override;
        unit *copy(unit *u) override;

        nga_edge *enga(unit *node, bool init) override;
        nga_edge *enga(unit *node, unit *u) override;
        nga_edge *connect(nga_status *a, nga_status *b) override;

    private:
        nga_status *status();
        void add_edge(nga_edge_list *&list, nga_edge *edge);
        void remove_edge(nga_edge_list *&list, nga_edge_list *edge);
        const char *label(unit *focused, bool front);
        void label(unit *node, unit *parent, unit *focused, bool front, std::ostream &os);
        void *disconnect(nga_status *status);

    public:
        void gen(const unit_rule &sym);
        void dump(std::ostream &os);

    private:
        void gen_nga();
        void check_nga();
        static nga_edge *conv_nga(unit *u);
        nga_status *delete_epsilon(nga_edge *edge);

    private:
        const char *str(const string_t &s);

    private:
        memory_pool<UNIT_NODE_MEM> nodes;
        std::unordered_set<std::string> strings;
        std::vector<std::string> labels;
        std::map<std::string, nga_rule> rules;
        unit_rule *current_rule{nullptr};
    };
};


#endif //CLIBPARSER_CUNIT_H
