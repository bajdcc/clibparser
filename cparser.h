//
// Project: CMiniLang
// Author: bajdcc
//
#ifndef CMINILANG_PARSER_H
#define CMINILANG_PARSER_H

#include "types.h"
#include "clexer.h"
#include "cast.h"
#include "cunit.h"

namespace clib {

    struct backtrace_t {
        int lexer_index;
        std::vector<int> state_stack;
        std::vector<ast_node *> ast_stack;
        int current_state;
        int coll_index;
        std::vector<int> trans_ids;
    };

    class cparser {
    public:
        explicit cparser(const string_t &str);
        ~cparser() = default;

        cparser(const cparser &) = delete;
        cparser &operator=(const cparser &) = delete;

        ast_node *parse();
        ast_node *root() const;

    private:
        void next();

        void gen();
        void program();
        ast_node *terminal();

        bool valid_trans(const pda_trans &trans) const;
        void do_trans(int state, const pda_trans &trans);
        bool LA(unit *u) const;

    private:
        void expect(bool, const string_t &);
        void match_keyword(keyword_t);
        void match_operator(operator_t);
        void match_type(lexer_t);
        void match_number();
        void match_integer();

        void error(const string_t &);

    private:
        lexer_t base_type{l_none};
        std::vector<int> state_stack;
        std::vector<ast_node *> ast_stack;
        std::vector<ast_node *> ast_cache;
        int ast_cache_index{0};
        std::vector<ast_node *> ast_coll_cache;

    private:
        cunit unit;
        clexer lexer;
        cast ast;
    };
}
#endif //CMINILANG_PARSER_H