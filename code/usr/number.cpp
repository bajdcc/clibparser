#include "/include/proc"
#include "/include/memory"
#include "/include/string"
#include "/include/io"
void error(char *s) {
    set_fg(240, 0, 0);
    put_string(s);
    restore_fg();
}
struct big_number {
    char *text;
    int capacity;
    int length;
};
big_number new_number() {
    big_number s;
    s.text = malloc(16);
    s.capacity = 16;
    s.length = 0;
    return s;
}
big_number new_number2(int n) {
    big_number s;
    s.text = malloc(n);
    memset(s.text, 0, n);
    s.capacity = n;
    s.length = n;
    return s;
}
void append_number(big_number *s, char c) {
    if (s->length >= s->capacity - 1) {
        s->capacity <<= 1;
        char *new_text = malloc(s->capacity);
        strncpy(new_text, s->text, s->length);
        free(s->text);
        s->text = new_text;
    }
    (s->text)[s->length++] = c;
}
big_number get_number(char *s) {
    big_number n = new_number();
    char *str = s;
    while (*s) {
        if (*s >= '0' && *s <= '9') {
            append_number(&n, *s - '0');
        } else {
            error("[ERROR] Invalid number: ");
            error(str);
            error("\n");
            exit(3);
        }
        s++;
    }
    return n;
}
void print_number(big_number s) {
    int i;
    for (i = 0; i < s.length; ++i) {
        put_char('0' + s.text[i]);
    }
}
void align2(big_number *a, big_number *b) {
    char *new_text = malloc(b->capacity);
    memset(new_text, 0, b->length - a->length);
    strncpy(new_text + b->length - a->length, a->text, a->length);
    free(a->text);
    a->text = new_text;
    a->length = b->length;
    a->capacity = b->capacity;
}
void align(big_number *a, big_number *b) {
    if (a->length == b->length)
        return;
    if (a->length < b->length)
        align2(a, b);
    else
        align2(b, a);
}
big_number plus(big_number *a, big_number *b) {
    big_number c = new_number2(a->length);
    int i, acc = 0;
    for (i = a->length - 1; i >= 0; --i) {
        c.text[i] = a->text[i] + b->text[i] + acc;
        if (c.text[i] >= 10) {
            c.text[i] -= 10;
            acc = 1;
        } else {
            acc = 0;
        }
    }
    return c;
}
int main(int argc, char **argv) {
    if (argc != 4) {
        error("[ERROR] Invalid arguments. Required: [+-*/] BigNumber1 BigNumber2\n");
        exit(1);
    }
    big_number a = get_number(argv[2]);
    big_number b = get_number(argv[3]);
    align(&a, &b);
    switch (argv[1][0]) {
        default:
            error("[ERROR] Invalid operator. Required: [+-*/]\n");
            exit(2);
            break;
        case '+':
            print_number(plus(&a, &b));
            break;
        case '-':
            break;
        case '*':
            break;
        case '/':
            break;
    }
    return 0;
}