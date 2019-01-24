//
// Project: cliblisp
// Created by bajdcc
//

#ifndef CLIBLISP_CGUI_H
#define CLIBLISP_CGUI_H

#include <array>
#include <deque>
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
#define GUI_INPUT_CARET 15

namespace clib {

    class cgui {
    public:
        cgui();
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
        void resize(int rows, int cols);

        void input_set(bool valid);
        void input(unsigned char c);

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
        std::vector<char> buffer;
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
        int rows{GUI_ROWS};
        int cols{GUI_COLS};
        int size{GUI_SIZE};
        bool input_state{false};
        int input_ticks{0};
        bool input_caret{false};
        std::vector<char> input_string;
    };
}

#endif //CLIBLISP_CGUI_H
