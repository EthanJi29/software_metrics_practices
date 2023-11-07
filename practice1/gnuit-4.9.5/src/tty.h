/* tty.h -- Data structures & function prototypes for tty.c stuff.  */

/* Copyright (C) 1993-2000, 2006-2007 Free Software Foundation, Inc.

 This file is part of gnuit.

 gnuit is free software: you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published
 by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.

 gnuit is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public
 License along with this program. If not, see
 http://www.gnu.org/licenses/. */

/* Written by Tudor Hulubei and Andrei Pitis.  */

#ifndef _GIT_TTY_H
#define _GIT_TTY_H


#include <sys/types.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#ifdef HAVE_POSIX_TTY
#include <termios.h>
#else
#ifdef HAVE_SYSTEMV_TTY
#include <termio.h>
#else
#include <sgtty.h>
#endif /* HAVE_SYSTEMV_TTY */
#endif /* HAVE_POSIX_TTY */

#include "window.h"


#define OFF                     0
#define ON                      1


/* Color constants.  */
#define BLACK		0
#define RED		1
#define GREEN		2
#define YELLOW		3
#define BLUE		4
#define MAGENTA		5
#define CYAN		6
#define WHITE		7


/* Some key aliases.  */
#define key_CTRL_G              0x07	/* Ctrl-G	*/
#define key_CTRL_Z              0x1a	/* Ctrl-Z	*/
#define key_CTRL_SPACE		0xff	/* Ctrl-Space	*/
#define key_ENTER               0x0a	/* Enter	*/
#define key_BACKSPACE           0x7f	/* Backspace	*/
#define key_TAB                 0x09	/* Tab		*/
#define key_ESC			0x1b	/* Esc		*/

/* The interrupt character.  */
#define key_INTERRUPT           key_CTRL_G
#define key_SUSPEND             key_CTRL_Z


/* Terminal modes.  */
#define TTY_CANONIC     0
#define TTY_NONCANONIC  1


/* Terminal input modes.  */
#define TTY_RESTRICTED_INPUT    0
#define TTY_FULL_INPUT          1


extern void (* tty_enter_idle_hook) PROTO (());
extern void (* tty_exit_idle_hook) PROTO (());


typedef struct tag_tty_key_t
{
    unsigned char *key_seq;
    struct tag_tty_key_t *next;
    void *aux_data;
} tty_key_t;


typedef unsigned char tty_status_t;


extern int tty_lines;
extern int tty_columns;
extern char *tty_type;
extern char *tty_device;

extern void tty_init PROTO ((int));
extern void tty_end PROTO ((char *));

extern void tty_resize PROTO (());

extern void tty_set_mode PROTO ((int));
extern int tty_get_mode PROTO (());
extern void tty_set_interrupt_char PROTO ((int));

extern void tty_clear PROTO (());
extern void tty_fill PROTO (());
extern void tty_touch PROTO (());
extern void tty_goto PROTO ((int, int));
extern void tty_get_cursor PROTO ((int *, int *));
extern void tty_brightness PROTO ((int));
extern void tty_foreground PROTO ((int));
extern void tty_background PROTO ((int));
extern void tty_colors PROTO ((int, int, int));
extern void tty_cursor PROTO ((int));
extern void tty_beep PROTO (());
extern void tty_defaults PROTO (());
extern void tty_save PROTO ((tty_status_t *));
extern void tty_restore PROTO ((tty_status_t *));
extern int tty_putc PROTO ((int));
extern int tty_puts PROTO ((char *, int));
extern int tty_getc PROTO (());
extern void tty_flush PROTO (());
extern void tty_update PROTO (());
extern void tty_get_screen PROTO ((char *));
extern void tty_put_screen PROTO ((char *));
extern int tty_get_color_index PROTO ((char *));
extern void tty_key_list_insert PROTO ((unsigned char *, void *));
extern void tty_key_search_restart PROTO (());
extern char *tty_key_human2machine PROTO ((unsigned char *));
extern unsigned char *tty_key_machine2human PROTO ((char *));
extern void tty_key_print PROTO ((char *));
extern void tty_key_print_async PROTO (());
extern char *tty_get_symbol_key_seq PROTO ((char *));
extern void tty_set_last_char_flag PROTO ((int));
extern int tty_set_optimization_level PROTO ((int));
extern void tty_update_title PROTO ((char *));

extern void tty_start_cursorapp PROTO (());
extern void tty_end_cursorapp PROTO (());

extern void tty_io_clear PROTO (());

/*
extern void tty_key_list_delete PROTO (());
*/

extern tty_key_t *tty_key_search PROTO ((char *));
extern tty_key_t *tty_get_key PROTO ((int *));
extern char *tty_get_previous_key_seq PROTO(());


#endif  /* _GIT_TTY_H */
