//
// Project: cliblisp
// Created by bajdcc
//

#include <GL/freeglut.h>
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include "cgui.h"
#include "cexception.h"

#define LOG_AST 0

#define ENTRY_FILE "/sys/entry"

namespace clib {

    cgui &cgui::singleton() {
        static clib::cgui gui;
        return gui;
    }

    string_t cgui::load_file(const string_t &name) {
        static string_t pattern{ R"(/([A-Za-z0-9_]+)/([A-Za-z0-9_.]+))" };
        static std::regex re(pattern);
        std::smatch res;
        if (std::regex_match(name, res, re)) {
            auto dir = res[1].str();
            auto file = res[2].str();
            auto path = "../code/" + dir + "/" + file + ".cpp";
            std::ifstream t(path);
            if (t) {
                std::stringstream buffer;
                buffer << t.rdbuf();
                return buffer.str();
            }
        }
        error("file not exists: " + name);
    }

    void cgui::draw(bool paused) {
        if (!paused) {
            for (int i = 0; i < ticks; ++i) {
                tick();
            }
        }
        draw_text();
    }

    void cgui::draw_text() {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);
        int width = GUI_COLS * GUI_FONT_W;
        int height = GUI_ROWS * GUI_FONT_H;
        gluOrtho2D(0, w, h, 0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glColor3f(0.9f, 0.9f, 0.9f);
        int x = std::max((w - width) / 2, 0);
        int y = std::max((h - height) / 2, 0);

        for (auto i = 0; i < GUI_ROWS; ++i) {
            glRasterPos2i(x, y);
            for (auto j = 0; j < GUI_COLS; ++j) {
                glutBitmapCharacter(GUI_FONT, buffer[i][j]);
            }
            y += GUI_FONT_H;
        }

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }

    void cgui::tick() {
        auto c = 0;
        if (running) {
            try {
                if (!vm->run(cycle, c)) {
                    running = false;
                    vm.reset();
                    gen.reset();
                }
            } catch (const cexception &e) {
                std::cout << "RUNTIME ERROR: " << e.msg << std::endl;
                vm.reset();
                gen.reset();
                running = false;
            }
        } else {
            if (!vm) {
                vm = std::make_unique<cvm>();
                if (compile(ENTRY_FILE) != -1) {
                    running = true;
                }
            }
        }
    }

    void cgui::put_char(char c) {
        if (c == '\n') {
            if (ptr_y == GUI_ROWS - 1) {
                new_line();
            } else {
                ptr_x = 0;
                ptr_y++;
            }
        } else if (c == '\b') {
            if (ptr_mx + ptr_my * GUI_COLS < ptr_x + ptr_y * GUI_COLS) {
                if (ptr_y == 0) {
                    if (ptr_x != 0) {
                        draw_char('\u0000');
                        ptr_x--;
                    }
                } else {
                    if (ptr_x != 0) {
                        draw_char('\u0000');
                        ptr_x--;
                    } else {
                        draw_char('\u0000');
                        ptr_x = GUI_COLS - 1;
                        ptr_y--;
                    }
                }
            }
        } else if (c == '\u0002') {
            ptr_x--;
            while (ptr_x >= 0) {
                draw_char('\u0000');
                ptr_x--;
            }
            ptr_x = 0;
        } else if (c == '\r') {
            ptr_x = 0;
        } else if (ptr_x == GUI_COLS - 1) {
            if (ptr_y == GUI_ROWS - 1) {
                draw_char(c);
                new_line();
            } else {
                draw_char(c);
                ptr_x = 0;
                ptr_y++;
            }
        } else {
            draw_char(c);
            ptr_x++;
        }
    }

    void cgui::put_int(int number) {
        static char str[256];
        sprintf(str, "%d", number);
        auto s = str;
        while (*s)
            put_char(*s++);
    }

    void cgui::new_line() {
        ptr_x = 0;
        for (int i = 0; i < GUI_ROWS - 1; ++i) {
            std::copy(buffer[i + 1].begin(), buffer[i + 1].end(), buffer[i].begin());
        }
        std::fill(buffer[GUI_ROWS - 1].begin(), buffer[GUI_ROWS - 1].end(), 0);
    }

    void cgui::draw_char(const char &c) {
        buffer[ptr_y][ptr_x] = c;
    }

    void cgui::error(const string_t &str) {
        std::stringstream ss;
        ss << "GUI ERROR: " << str;
        throw cexception(ss.str());
    }

    void cgui::set_cycle(int cycle) {
        this->cycle = cycle;
    }

    void cgui::set_ticks(int ticks) {
        this->ticks = ticks;
    }

    void cgui::record(int ms) {
        record_now = std::chrono::high_resolution_clock::now();
        waiting_ms = ms * 0.001;
    }

    bool cgui::reach() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<decimal>>(now - record_now).count() > waiting_ms;
    }

    int cgui::compile(const string_t &path) {
        try {
            auto code = load_file(path);
            gen.reset();
            auto root = p.parse(code, &gen);
#if LOG_AST
            cast::print(root, 0, std::cout);
#endif
            gen.gen(root);
            auto file = gen.file();
            cache.insert(std::make_pair(path, file));
            return vm->load(file);
        } catch (const cexception &e) {
            gen.reset();
            std::cout << "COMPILE ERROR: " << e.msg << std::endl;
            return -1;
        }
    }
}
