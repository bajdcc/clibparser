//
// Project: cliblisp
// Created by bajdcc
//

#ifndef CLIBLISP_CGUI_H
#define CLIBLISP_CGUI_H

#include <array>
#include <deque>
#include <chrono>
#include "types.h"
#include "cparser.h"
#include "cgen.h"

#define GUI_FONT GLUT_BITMAP_9_BY_15
#define GUI_FONT_W 9
#define GUI_FONT_H 15
#define GUI_ROWS 30
#define GUI_COLS 84
#define GUI_SIZE (GUI_ROWS * GUI_COLS)
#define GUI_CYCLES 1000
#define GUI_TICKS 1

namespace clib {

    class cgui {
    public:
        cgui();
        ~cgui() = default;

        cgui(const cgui &) = delete;
        cgui &operator=(const cgui &) = delete;

        void draw();

        void put_char(char c);

        void set_cycle(int cycle);
        void set_ticks(int ticks);

        void record();
        bool reach(const decimal &d);
        void control(int type);

    private:
        void tick();
        void draw_text();

        void new_line();
        inline void draw_char(const char &c);

    public:
        static cgui &singleton();

    private:
        std::array<std::array<char, GUI_COLS>, GUI_ROWS> buffer;
        std::deque<string_t> codes;
        cgen gen;
        cparser p;
        bool running{false};
        int cycle{ GUI_CYCLES };
        int ticks{ GUI_TICKS };
        int ptr_x{0};
        int ptr_y{0};
        int ptr_mx{0};
        int ptr_my{0};
        int continues{0};
        string_t current_code;
        std::chrono::system_clock::time_point record_now;
    };
}

#endif //CLIBLISP_CGUI_H
