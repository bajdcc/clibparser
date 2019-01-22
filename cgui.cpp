//
// Project: cliblisp
// Created by bajdcc
//

#include <GL/freeglut.h>
#include <iostream>
#include "cgui.h"
#include "cexception.h"

#define LOG_AST 0

namespace clib {

    cgui::cgui() {
        auto cs = std::vector<string_t>{
            R"(
enum INTR_TABLE {
    INTR_PUT_CHAR = 0,
    INTR_PUT_NUMBER = 1,
    INTR_SLEEP_RECORD = 100,
    INTR_SLEEP_REACH = 101,
};
int sleep(int ms) {
    ms;
    interrupt 100;
    interrupt 101;
}
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
int fib2(int i) {
    return i > 2 ? fib(i - 1) + fib(i - 2) : 1;
}
int fib3(int i) {
    int a[3], j;
    a[0] = a[1] = 1;
    for (j = 1; j < i; ++j)
        a[2] = a[0] + a[1], a[0] = a[1], a[1] = a[2];
    return a[0];
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
    for (i = 1, s = 0; i <= n ; ++i) {
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
int welcome() {
    put_string(" ________  ________        ___  ________  ________  ________     \n");
    put_string("|\\   __  \\|\\   __  \\      |\\  \\|\\   ___ \\|\\   ____\\|\\   ____\\    \n");
    put_string("\\ \\  \\|\\ /\\ \\  \\|\\  \\     \\ \\  \\ \\  \\_|\\ \\ \\  \\___|\\ \\  \\___|    \n");
    put_string(" \\ \\   __  \\ \\   __  \\  ___\\ \\  \\ \\  \\ \\\\ \\ \\  \\    \\ \\  \\       \n");
    put_string("  \\ \\  \\|\\  \\ \\  \\ \\  \\|\\  \\\\_\\  \\ \\  \\_\\\\ \\ \\  \\____\\ \\  \\_____\n");
    put_string("   \\ \\_______\\ \\__\\ \\__\\ \\________\\ \\_______\\ \\_______\\ \\_______\\\n");
    put_string("    \\|_______|\\|__|\\|__|\\|________|\\|_______|\\|_______|\\|_______|\n");
    put_string("\n\n");
    put_string("Welcome to @clibos system by bajdcc!");
    put_string("\n\n");
}
enum TEST {
    TEST_IF,
    TEST_TRIOP,
    TEST_ARRAY,
    TEST_WHILE,
    TEST_FOR,
    TEST_DO,
};
int test(int i) {
    switch (i) {
        case TEST_IF:
            put_string("fib(10):   "); put_int(fib(10));
            break;
        case TEST_TRIOP:
            put_string("fib2(10):  "); put_int(fib2(10));
            break;
        case TEST_ARRAY:
            put_string("fib3(10):  "); put_int(fib3(10));
            break;
        case TEST_WHILE:
            put_string("sum(100):  "); put_int(sum(100));
            break;
        case TEST_FOR:
            put_string("sum2(100): "); put_int(sum2(100));
            break;
        case TEST_DO:
            put_string("sum3(100): "); put_int(sum3(100));
            break;
        default:
            put_string("undefined task");
            break;
    }
    put_string("\n"); sleep(100);
}
int main(int argc, char **argv) {
    welcome();
    int i;
    for (i = TEST_IF; i <= TEST_DO; ++i)
        test(i);
    return 0;
}
)",
        };
        std::copy(cs.begin(), cs.end(), std::back_inserter(codes));
    }

    cgui &cgui::singleton() {
        static clib::cgui gui;
        return gui;
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
#if LOG_AST
                    cast::print(root, 0, std::cout);
#endif
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

    void cgui::record(int ms) {
        record_now = std::chrono::high_resolution_clock::now();
        waiting_ms = ms * 0.001;
    }

    bool cgui::reach() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<decimal>>(now - record_now).count() > waiting_ms;
    }

    void cgui::control(int type) {
        if (type == 0) { // continue
            continues++;
        } else if (type == 1) { // break
            continues = 0;
        }
    }
}
