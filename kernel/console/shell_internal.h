#ifndef SHELL_INTERNAL_H
#define SHELL_INTERNAL_H
#include "shell.h"
#include <stdbool.h>
const char *parse_token(const char *, char *, int);
void copy_rest(const char *, char *, int);
int parse_int(const char *);
bool grep_file(const char *, const char *);
void handle_asm_command(const char *);
void shell_echo(const char *);
void show_mouse_state(void);
void print_prompt(void);
void print_help(void);
const char *layout_name(int);
void fastfetch(void);
void ls_long(const char *);
void ls_short(const char *);
void tree_print(const char *, const char *, int, int);
#endif
