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
        cgui() = default;
        ~cgui() = default;

        cgui(const cgui &) = delete;
        cgui &operator=(const cgui &) = delete;

        void draw(bool paused);
        int compile(const string_t &path, const std::vector<string_t> &args);

        void put_string(const string_t &str);
        void put_char(char c);
        void put_int(int number);

        void set_cycle(int cycle);
        void set_ticks(int ticks);

        void record(int ms);
        bool reach() const;

    private:
        void tick();
        void draw_text();

        void new_line();
        inline void draw_char(const char &c);

        static void error(const string_t &);

    public:
        static cgui &singleton();

        static string_t load_file(const string_t &name);

    private:
        std::array<std::array<char, GUI_COLS>, GUI_ROWS> buffer;
        cgen gen;
        cparser p;
        std::unique_ptr<cvm> vm;
        std::unordered_map<string_t, std::vector<byte>> cache;
        bool running{false};
        bool exited{false};;
        int cycle{ GUI_CYCLES };
        int ticks{ GUI_TICKS };
        int ptr_x{0};
        int ptr_y{0};
        int ptr_mx{0};
        int ptr_my{0};
        std::chrono::system_clock::time_point record_now;
        decimal waiting_ms{0};
    };
}

#endif //CLIBLISP_CGUI_H
