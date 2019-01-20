//
// Project: cliblisp
// Created by bajdcc
//

#include <GL/freeglut.h>
#include <iostream>
#include "cgui.h"
#include "cexception.h"

namespace clib {

    cgui::cgui() {
        auto cs = std::vector<string_t>{
            R"(
int put_char(char c) {
    c;
    interrupt 0;
}
int put_string(char *text) {
    while (put_char(*text++));
}
int put_int(int number) {
    number;
    interrupt 1;
}
int fib(int i) {
    if (i > 2)
        return fib(i - 1) + fib(i - 2);
    else
        return 1;
}
int sum(int i) {
    int s = 0;
    while (i > 0) {
        s += i--;
    }
    return s;
}
int sum2(int n) {
    int i, s;
    for (i = 1, s = 0; i <= n; ++i) {
        s += i;
    }
    return s;
}
int sum3(int i) {
    int s = 0;
    do {
        s += i--;
    } while (i > 0);
    return s;
}
int main(int argc, char **argv){
    put_string("fib(10): ");   put_int(fib(10));   put_string("\n");
    put_string("sum(100): ");  put_int(sum(100));  put_string("\n");
    put_string("sum2(100): "); put_int(sum2(100)); put_string("\n");
    put_string("sum3(100): "); put_int(sum3(100)); put_string("\n");
}
)",
        };
        std::copy(cs.begin(), cs.end(), std::back_inserter(codes));
    }

    cgui &cgui::singleton() {
        static clib::cgui gui;
        return gui;
    }

    void cgui::draw() {
        for (int i = 0; i < ticks; ++i) {
            tick();
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
        auto error = false;
        if (running) {
            try {
                if (!gen.eval(GUI_CYCLES, c)) {
                    running = false;
                    gen.reset();
                }
            } catch (const cexception &e) {
                error = true;
                std::cout << "RUNTIME ERROR: " << e.msg << std::endl;
                gen.reset();
                running = false;
            }
        } else {
            if (!codes.empty()) {
                current_code = codes.front();
                codes.pop_front();
                try {
                    auto root = p.parse(current_code, &gen);
                    //cast::print(root, 0, std::cout);
                    gen.gen(root);
                    if (gen.eval(GUI_CYCLES, c)) {
                        running = true;
                    } else {
                        gen.reset();
                    }
                } catch (const cexception &e) {
                    error = true;
                    std::cout << "RUNTIME ERROR: " << e.msg << std::endl;
                }
            }
        }
        if (continues > 0) {
            if (error) {
                continues = 0;
                current_code = "";
            } else {
                codes.push_front(current_code);
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

    void cgui::set_cycle(int cycle) {
        this->cycle = cycle;
    }

    void cgui::set_ticks(int ticks) {
        this->ticks = ticks;
    }

    void cgui::record() {
        record_now = std::chrono::high_resolution_clock::now();
    }

    bool cgui::reach(const decimal &d) {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<decimal>>(now - record_now).count() > d;
    }

    void cgui::control(int type) {
        if (type == 0) { // continue
            continues++;
        } else if (type == 1) { // break
            continues = 0;
        }
    }
}
