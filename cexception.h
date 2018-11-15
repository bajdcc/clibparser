//
// Project: clibparser
// Author: bajdcc
//
#ifndef CLIBVM_EXCEPTION_H
#define CLIBVM_EXCEPTION_H

#include <exception>
#include "types.h"

namespace clib {
    class cexception : public std::exception {
    public:
        explicit cexception(const string_t &msg) noexcept;
        ~cexception() = default;

        cexception(const cexception& e) = default;
        cexception &operator = (const cexception& e) = default;

        string_t msg;
    };
}

#endif
