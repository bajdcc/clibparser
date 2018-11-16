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
#define UNIT_TYPE_CHAR_BASE 0x1000
#define UNIT_TYPE_KEYWORD_BASE 0x2000
#define UNIT_TYPE_OPERATOR_BASE 0x3000

namespace clib {

    enum unit_t {
        u_none,
        u_token,
        u_rule,
        u_sequence,
        u_branch,
        u_optional,
    };

    class unit_builder;

    struct unit {
        unit_t type;
        unit *next;
        unit_builder *builder;

        unit &operator=(const unit &u);
        unit &operator+(const unit &u);
        unit &operator|(const unit &u);
    };

    struct unit_token : public unit {
        lexer_t type;
        union {
            operator_t op;
            keyword_t keyword;
        } value;

        unit& operator,(lexer_t type);
        unit& operator,(operator_t op);
        unit& operator,(keyword_t keyword);
    };

    struct unit_collection : public unit {
        unit *child;
    };

    struct unit_rule : public unit_collection {
        const char *s;
    };

    class unit_builder {
    };

    // 文法表达式
    class cunit {
    public:
        cunit() = default;
        ~cunit() = default;

        cunit(const cunit &) = delete;
        cunit &operator=(const cunit &) = delete;

        unit &token(const lexer_t &type);
        unit &token(const operator_t &op);
        unit &token(const keyword_t &keyword);
        unit &rule(const string_t &s);

    private:
        const char *str(const string_t &s);

    private:
        memory_pool<UNIT_NODE_MEM> nodes;
        std::unordered_set<std::string> strings;
        std::unordered_map<std::string, unit *> rules;
    };
};


#endif //CLIBPARSER_CUNIT_H
