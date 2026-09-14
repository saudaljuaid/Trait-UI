/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TRAIT_TERMINAL_H
#define TRAIT_TERMINAL_H

#include <stdbool.h>
#include <stdint.h>

#include <trait/surface.h>
#include <trait/window.h>

/*
 * lxterminal, which is LXDE's terminal.
 *
 * Black ground, light grey text, a monospace face and a block cursor -
 * the old look rather than the modern one, which means no transparency,
 * no rounded anything and no colour beyond what the shell itself emits.
 *
 * IT ANSWERS WHAT IS TYPED AT IT.  A terminal that prints a prompt and
 * ignores the keyboard is a picture of a terminal, so there is a command
 * table here and an unknown command says so the way a shell does.
 */

#define TRAIT_TERM_COLUMNS 80U
#define TRAIT_TERM_ROWS 24U
#define TRAIT_TERM_LINE_BYTES 96U

void trait_terminal_reset(void);
void trait_terminal_print(const char *line);
/* Runs a command line: echoes it after the prompt, then its output. */
void trait_terminal_run(const char *command);
uint32_t trait_terminal_row_count(void);
const char *trait_terminal_row(uint32_t at);

void trait_terminal_draw(struct trait_surface *surface,
    const struct trait_window *window);

bool trait_terminal_self_test(void);

#endif /* TRAIT_TERMINAL_H */
