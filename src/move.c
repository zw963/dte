#include "move.h"
#include "view.h"
#include "buffer.h"
#include "indent.h"
#include "uchar.h"

enum char_type {
    CT_SPACE,
    CT_NEWLINE,
    CT_WORD,
    CT_OTHER,
};

void move_to_preferred_x(int preferred_x)
{
    unsigned int tw = buffer->options.tab_width;
    struct lineref lr;
    long i = 0;
    unsigned int x = 0;

    view->preferred_x = preferred_x;

    block_iter_bol(&view->cursor);
    fill_line_ref(&view->cursor, &lr);

    if (buffer->options.emulate_tab && view->preferred_x < lr.size) {
        int iw = buffer->options.indent_width;
        int ilevel = view->preferred_x / iw;

        for (i = 0; i < lr.size && lr.line[i] == ' '; i++) {
            if (i + 1 == (ilevel + 1) * iw) {
                // Force cursor to beginning of the indentation level
                view->cursor.offset += ilevel * iw;
                return;
            }
        }
        i = 0;
    }

    while (x < view->preferred_x && i < lr.size) {
        unsigned int u = lr.line[i++];

        if (u < 0x80) {
            if (!u_is_ctrl(u)) {
                x++;
            } else if (u == '\t') {
                x = (x + tw) / tw * tw;
            } else if (u == '\n') {
                break;
            } else {
                x += 2;
            }
        } else {
            int next = i;
            i--;
            u = u_get_nonascii(lr.line, lr.size, &i);
            x += u_char_width(u);
            if (x > view->preferred_x) {
                i = next;
                break;
            }
        }
    }
    if (x > view->preferred_x)
        i--;
    view->cursor.offset += i;
}

void move_cursor_left(void)
{
    unsigned int u;

    if (buffer->options.emulate_tab) {
        int size = get_indent_level_bytes_left();
        if (size) {
            block_iter_back_bytes(&view->cursor, size);
            view_reset_preferred_x(view);
            return;
        }
    }

    buffer_prev_char(&view->cursor, &u);
    view_reset_preferred_x(view);
}

void move_cursor_right(void)
{
    unsigned int u;

    if (buffer->options.emulate_tab) {
        int size = get_indent_level_bytes_right();
        if (size) {
            block_iter_skip_bytes(&view->cursor, size);
            view_reset_preferred_x(view);
            return;
        }
    }

    buffer_next_char(&view->cursor, &u);
    view_reset_preferred_x(view);
}

void move_bol(void)
{
    block_iter_bol(&view->cursor);
    view_reset_preferred_x(view);
}

void move_eol(void)
{
    block_iter_eol(&view->cursor);
    view_reset_preferred_x(view);
}

void move_up(int count)
{
    int x = view_get_preferred_x(view);

    while (count > 0) {
        if (!block_iter_prev_line(&view->cursor))
            break;
        count--;
    }
    move_to_preferred_x(x);
}

void move_down(int count)
{
    int x = view_get_preferred_x(view);

    while (count > 0) {
        if (!block_iter_eat_line(&view->cursor))
            break;
        count--;
    }
    move_to_preferred_x(x);
}

void move_bof(void)
{
    block_iter_bof(&view->cursor);
    view_reset_preferred_x(view);
}

void move_eof(void)
{
    block_iter_eof(&view->cursor);
    view_reset_preferred_x(view);
}

void move_to_line(struct view *v, int line)
{
    block_iter_goto_line(&v->cursor, line - 1);
    v->center_on_scroll = true;
}

void move_to_column(struct view *v, int column)
{
    block_iter_bol(&v->cursor);
    while (column-- > 1) {
        unsigned int u;

        if (!buffer_next_char(&v->cursor, &u))
            break;
        if (u == '\n') {
            buffer_prev_char(&v->cursor, &u);
            break;
        }
    }
    view_reset_preferred_x(v);
}

static enum char_type get_char_type(unsigned int u)
{
    if (u == '\n')
        return CT_NEWLINE;
    if (u_is_space(u))
        return CT_SPACE;
    if (u_is_word_char(u))
        return CT_WORD;
    return CT_OTHER;
}

static bool get_current_char_type(struct block_iter *bi, enum char_type *type)
{
    unsigned int u;

    if (!buffer_get_char(bi, &u))
        return false;

    *type = get_char_type(u);
    return true;
}

static long skip_fwd_char_type(struct block_iter *bi, enum char_type type)
{
    long count = 0;
    unsigned int u;

    while (buffer_next_char(bi, &u)) {
        if (get_char_type(u) != type) {
            buffer_prev_char(bi, &u);
            break;
        }
        count += u_char_size(u);
    }
    return count;
}

static long skip_bwd_char_type(struct block_iter *bi, enum char_type type)
{
    long count = 0;
    unsigned int u;

    while (buffer_prev_char(bi, &u)) {
        if (get_char_type(u) != type) {
            buffer_next_char(bi, &u);
            break;
        }
        count += u_char_size(u);
    }
    return count;
}

long word_fwd(struct block_iter *bi, bool skip_non_word)
{
    long count = 0;
    enum char_type type;

    while (1) {
        count += skip_fwd_char_type(bi, CT_SPACE);
        if (!get_current_char_type(bi, &type))
            return count;

        if (count && (!skip_non_word || (type == CT_WORD || type == CT_NEWLINE)))
            return count;

        count += skip_fwd_char_type(bi, type);
    }
}

long word_bwd(struct block_iter *bi, bool skip_non_word)
{
    long count = 0;
    enum char_type type;
    unsigned int u;

    do {
        count += skip_bwd_char_type(bi, CT_SPACE);
        if (!buffer_prev_char(bi, &u))
            return count;

        type = get_char_type(u);
        count += u_char_size(u);
        count += skip_bwd_char_type(bi, type);
    } while (skip_non_word && type != CT_WORD && type != CT_NEWLINE);
    return count;
}
