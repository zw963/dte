#ifndef EDIT_H
#define EDIT_H

unsigned int prepare_selection(void);

void delete(unsigned int len, int move_after);
void cut(unsigned int len, int is_lines);
void copy(unsigned int len, int is_lines);
void select_start(int is_lines);
void select_end(void);
void paste(void);
void join_lines(void);
void erase_word(void);
void shift_lines(int count);
void move_left(int count);
void move_right(int count);
void move_bol(void);
void move_eol(void);

#endif