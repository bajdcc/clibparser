//
// Project: clibparser
// Created by CC
//

#ifndef CLIBPARSER_CUNIT_H
#define CLIBPARSER_CUNIT_H


#include <string>
#include <unordered_map>
#include <unordered_set>
#include "memory.h"

#define UNIT_NODE_MEM (2 * 1024)

namespace clib {

    enum unit_t {
        u_none,
        u_token,
        u_token_ref,
        u_rule,
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

    class unit_builder {
    public:
        virtual unit_collection &append(unit *collection, unit *child) = 0;
        virtual unit_collection &merge(unit *a, unit *b) = 0;
        virtual unit &collection(unit *a, unit *b, unit_t type) = 0;
    };

    // 文法表达式
    class cunit : unit_builder {
    public:
        cunit() = default;
        ~cunit() = default;

        cunit(const cunit &) = delete;
        cunit &operator=(const cunit &) = delete;

        unit &token(const lexer_t &type);
        unit &token(const operator_t &op);
        unit &token(const keyword_t &keyword);
        unit &rule(const string_t &s);

        unit *copy(unit *u);
        unit_collection &append(unit *collection, unit *child) override;
        unit_collection &merge(unit *a, unit *b) override;
        unit_collection &collection(unit *a, unit *b, unit_t type) override;

        void dump(std::ostream &os);

    private:
        const char *str(const string_t &s);

    private:
        memory_pool<UNIT_NODE_MEM> nodes;
        std::unordered_set<std::string> strings;
        std::unordered_map<std::string, unit *> rules;
    };
};


#endif //CLIBPARSER_CUNIT_H
