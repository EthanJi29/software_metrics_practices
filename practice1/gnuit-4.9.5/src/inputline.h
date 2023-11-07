/* inputline.h -- Prototypes for the functions in inputline.c.  */

/* Copyright (C) 1993-1999, 2006-2007 Free Software Foundation, Inc.

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

#ifndef _GIT_INPUTLINE_H
#define _GIT_INPUTLINE_H


#include <sys/types.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include "stdc.h"
#include "window.h"


#define IL_RECORD       0
#define IL_PREVIOUS     1
#define IL_NEXT         2


#define IL_DONT_STORE   0
#define IL_DONT_KILL    0
#define IL_STORE        1
#define IL_KILL         2


/* Some #defines used in external input line interfaces.  */
#define IL_FREEZED	 0
#define IL_EDIT		 1
#define IL_MOVE		 2
#define IL_BEEP		 4
#define IL_ERROR	 8
#define IL_SAVE		16
#define IL_HOME		32

#define IL_ISEARCH_ACTION_FAILED       -1
#define IL_ISEARCH_ACTION_NONE          0
#define IL_ISEARCH_ACTION_DECREASE      1
#define IL_ISEARCH_ACTION_RETRY         2
#define IL_ISEARCH_ACTION_INCREASE      3

#define IL_UPDATE        1


typedef struct
{
    window_t *window;		/* The input line window.  */
    int echo;			/* The echo flag.  */
    int error;			/* Use error-like output.  */
    int last_operation;		/* The last basic edit operation performed.  */
    size_t point;		/* The point position.  */
    size_t mark;		/* The mark position.  */
    size_t columns;		/* The number of columns.  */
    size_t line;		/* The input line's location.  */
    size_t length;		/* The total length of the input line.  */
    size_t static_length;	/* The static text length.  */
    size_t dynamic_length;	/* The dynamic text length.  */
    size_t size;		/* The current buffer size.  */
    char *buffer;		/* The data buffer.  */
    char *kill_ring;		/* The kill ring.  */
    char *history_file;		/* The name of the history file.  */
} input_line_t;


extern input_line_t *il;

extern void il_init PROTO (());
extern void il_end PROTO (());
extern void il_free PROTO ((input_line_t *));
extern void il_resize PROTO ((int, int));
extern input_line_t *il_save PROTO (());
extern void il_restore PROTO ((input_line_t *));
extern size_t il_point PROTO (());
extern int il_echo PROTO ((int));
extern int il_is_empty PROTO (());
extern void il_set_mark PROTO (());
extern void il_kill_region PROTO (());
extern void il_kill_ring_save PROTO (());
extern void il_yank PROTO (());
extern void il_exchange_point_and_mark PROTO (());
extern void il_backward_char PROTO (());
extern void il_forward_char PROTO (());
extern void il_backward_word PROTO (());
extern void il_forward_word PROTO (());
extern void il_beginning_of_line PROTO (());
extern void il_end_of_line PROTO (());
extern void il_insert_char PROTO ((int));
extern void il_delete_char PROTO (());
extern void il_backward_delete_char PROTO (());
extern void il_kill_word PROTO (());
extern void il_backward_kill_word PROTO (());
extern void il_reset_line PROTO (());
extern void il_kill_line PROTO ((int));
extern void il_kill_to_beginning_of_line PROTO (());
extern void il_kill_to_end_of_line PROTO (());
extern void il_just_one_space PROTO (());
extern void il_delete_horizontal_space PROTO (());
extern void il_downcase_word PROTO (());
extern void il_upcase_word PROTO (());
extern void il_capitalize_word PROTO (());
extern void il_set_static_text PROTO ((char *));
extern void il_insert_text PROTO ((char *));
extern void il_update_point PROTO (());
extern void il_update PROTO (());
extern int il_get_contents PROTO ((char **));
extern void il_message PROTO ((char *));
extern void il_set_error_flag PROTO ((int));
extern void il_history PROTO ((int));


#endif  /* _GIT_INPUTLINE_H */
