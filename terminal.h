/*
 * Terminal handling functions
 * Manages terminal modes and signals
 */
#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"

/* put terminal into a raw mode */
int tty_raw(int fd);

/* restore terminal's mode */
int tty_reset(int fd);

/* signal catch error for main */
void sig_catch(int signo);

#endif /* TERMINAL_H */