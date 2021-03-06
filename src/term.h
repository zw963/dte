#ifndef TERM_H
#define TERM_H

#include "libc.h"
#include "key.h"

enum {
    STR_CAP_CMD_ac, // Pairs of block graphic characters to map alternate character set
    STR_CAP_CMD_ae, // End alternative character set
    STR_CAP_CMD_as, // Start alternative character set for block graphic characters
    STR_CAP_CMD_ce, // Clear to end of line
    STR_CAP_CMD_ke, // Turn keypad off
    STR_CAP_CMD_ks, // Turn keypad on
    STR_CAP_CMD_te, // End program that uses cursor motion
    STR_CAP_CMD_ti, // Begin program that uses cursor motion
    STR_CAP_CMD_ve, // Show cursor
    STR_CAP_CMD_vi, // Hide cursor

    NR_STR_CAPS
};

enum {
    COLOR_DEFAULT = -1,
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_GREY
};

enum {
    ATTR_BOLD = 0x01,
    ATTR_LOW_INTENSITY = 0x02,
    ATTR_ITALIC = 0x04,
    ATTR_UNDERLINE = 0x08,
    ATTR_BLINKING = 0x10,
    ATTR_REVERSE_VIDEO = 0x20,
    ATTR_INVISIBLE_TEXT = 0x40,
    ATTR_KEEP = 0x80,
};

struct term_keymap {
    int key;
    char *code;
};

// See termcap(5)
struct term_cap {
    // Boolean caps
    bool ut; // Can clear to end of line with bg color set

    // Integer caps
    int colors;

    // String caps
    char *strings[NR_STR_CAPS];

    // String caps (keys)
    struct term_keymap *keymap;
    int keymap_size;
};

struct term_color {
    short fg;
    short bg;
    unsigned short attr;
};

extern struct term_cap term_cap;

int term_init(const char *term);
void term_raw(void);
void term_cooked(void);

bool term_read_key(int *key);
char *term_read_paste(long *size);
void term_discard_paste(void);

int term_get_size(int *w, int *h);

const char *term_set_color(const struct term_color *color);

// Move cursor (x and y are zero based)
const char *term_move_cursor(int x, int y);

void term_read_caps(void);

#endif
