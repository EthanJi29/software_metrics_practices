/* inputline.c -- Input line management functions.  */

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <sys/types.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else /* !HAVE_STDLIB_H */
#include "ansi_stdlib.h"
#endif /* !HAVE_STDLIB_H */

#include <ctype.h>
#include <assert.h>

#include "xstring.h"
#include "xmalloc.h"
#include "configure.h"
#include "history.h"
#include "tty.h"
#include "window.h"
#include "inputline.h"
#include "tilde.h"
#include "misc.h"


extern int AnsiColors;


#define MAX_INPUT_HISTORY       512     /* big enough I suppose ... */


#define INPUTLINE_FIELDS        6

static char *InputLineFields[INPUTLINE_FIELDS] =
{
    "InputLineForeground",
    "InputLineBackground",
    "InputLineBrightness",
    "InputLineErrorForeground",
    "InputLineErrorBackground",
    "InputLineErrorBrightness",
};

#ifdef HAVE_LINUX
static int InputLineColors[INPUTLINE_FIELDS] =
{
    WHITE, BLACK, ON, RED, BLACK, ON
};
#else   /* !HAVE_LINUX */
static int InputLineColors[INPUTLINE_FIELDS] =
{
    WHITE, BLACK, ON, WHITE, BLACK, ON
};
#endif  /* !HAVE_LINUX */

#define InputLineForeground             InputLineColors[0]
#define InputLineBackground             InputLineColors[1]
#define InputLineBrightness             InputLineColors[2]
#define InputLineErrorForeground        InputLineColors[3]
#define InputLineErrorBackground        InputLineColors[4]
#define InputLineErrorBrightness        InputLineColors[5]


/* Basic input line operations.  */
#define IL_NO_OPERATION				 0
#define IL_BACKWARD_CHAR			 1
#define IL_FORWARD_CHAR				 2
#define IL_BACKWARD_WORD			 3
#define IL_FORWARD_WORD				 4
#define IL_BEGINNING_OF_LINE			 5
#define IL_END_OF_LINE				 6
#define IL_INSERT_CHAR				 7
#define IL_DELETE_CHAR				 8
#define IL_BACKWARD_DELETE_CHAR			 9
#define IL_KILL_WORD				10
#define IL_BACKWARD_KILL_WORD			11
#define IL_RESET_LINE				12
#define IL_KILL_LINE				13
#define IL_KILL_TO_BEGINNING_OF_LINE		14
#define IL_KILL_TO_END_OF_LINE			15
#define IL_JUST_ONE_SPACE			16
#define IL_DELETE_HORIZONTAL_SPACE		17
#define IL_DOWNCASE_WORD			18
#define IL_UPCASE_WORD				19
#define IL_CAPITALIZE_WORD			20
#define IL_SET_STATIC_TEXT			21
#define IL_INSERT_TEXT				22
#define IL_SET_MARK				23
#define IL_KILL_REGION				24
#define IL_KILL_RING_SAVE			25
#define IL_YANK					26
#define IL_EXCHANGE_POINT_AND_MARK		27


#define IL_RESIZE(il_new_buffer_size)			\
{							\
    il->size = il_new_buffer_size;			\
    il->buffer = xrealloc(il->buffer, il->size);	\
}


/* The input line.  */
input_line_t *il = NULL;


/*
 * Check if 'c' is a separator.
 */
static int
il_separator(c)
    char c;
{
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9') ||
	(c == '$')             ||
	(c == '%'))
	return 0;
    else
	return 1;
}


/*
 * Store the text between the mark and the point into the kill_ring if
 * flags & IL_STORE.  Kill it if flags & IL_KILL.
 */
static void
il_region_command(flags)
    int flags;
{
    size_t region_start;
    size_t region_end;
    size_t region_size;


    /* If the region is empty, preserve the previous kill_ring.  */
    if (il->mark == il->point)
	return;

    /* Get the region limits.  */
    if (il->mark > il->point)
    {
	region_start = il->point;
	region_end   = il->mark;
    }
    else
    {
	region_start = il->mark;
	region_end   = il->point;
    }

    region_size = region_end - region_start;

    if (flags & IL_STORE)
    {
	if (il->kill_ring)
	    xfree(il->kill_ring);

	il->kill_ring = xmalloc(region_size + 1);
	memcpy(il->kill_ring, il->buffer + region_start, region_size);
	il->kill_ring[region_size] = '\0';
    }

    if (flags & IL_KILL)
    {
	il->dynamic_length -= region_size;
	il->length         -= region_size;
	il->point           = region_start;
	il->mark            = region_start;
	strcpy(il->buffer + region_start, il->buffer + region_end);
	IL_RESIZE(il->length + 1);
    }
}


/*
 * Free an input line structure.
 */
void
il_free(some_il)
    input_line_t *some_il;
{
    if (some_il == NULL)
	return;

    if (some_il->buffer)
	xfree(some_il->buffer);

    if (some_il->kill_ring)
	xfree(some_il->kill_ring);

    xfree(some_il);
}


/*
 * Compute the amount of scrolling when the point is at the end of the
 * input line.  It should be about 25% of the dynamic part of the
 * input line, but if the input line shrinks (as a result of a
 * SIGWINCH) it should not become less than 1.
 */
static size_t
il_compute_scroll()
{
    return max((il->columns - il->static_length) / 4,  1);
}


/*
 * The input_line constructor.
 */
void
il_init()
{
    char *data;

    il = (input_line_t *)xmalloc(sizeof(input_line_t));

    il->echo = 1;
    il->error = 0;
    il->buffer = NULL;
    il->kill_ring = NULL;
    il->columns = 0;
    il->line = 0;
    il_reset_line();
    il->window = window_init();


    use_section("[GITFM-Setup]");

    configuration_getvarinfo("HistoryFile", &data, 1, DO_SEEK);
    il->history_file = tilde_expand(data ? data : "~/.githistory");

    use_section(AnsiColors ? color_section : monochrome_section);

    get_colorset_var(InputLineColors, InputLineFields, INPUTLINE_FIELDS);


    /* Initialize the history library stuff...  */
    using_history();
    read_history(il->history_file);
    while (next_history());
    stifle_history(MAX_INPUT_HISTORY);
}


/*
 * Input line destructor.
 */
void
il_end()
{
    write_history(il->history_file);
    window_end(il->window);

    il_free(il);
    il = NULL;
}


/*
 * Resize the input line.
 */
void
il_resize(_columns, _line)
    int _columns, _line;
{
    il->columns = _columns;
    il->line = _line;
    window_resize(il->window, 0, _line, 1, _columns);
}


/*
 * Save the entire input_line structure.  Returns a pointer to the saved
 * structure.
 */
input_line_t *
il_save()
{
    input_line_t *saved_il = (input_line_t *)xmalloc(sizeof(input_line_t));

    *saved_il = *il;

    if (saved_il->buffer)
    {
	il->buffer = xmalloc(saved_il->size);
	memcpy(il->buffer, saved_il->buffer, saved_il->size);
    }

    if (saved_il->kill_ring)
	il->kill_ring = xstrdup(saved_il->kill_ring);

    return saved_il;
}


/*
 * Restore the input_line.
 */
void
il_restore(saved_il)
    input_line_t *saved_il;
{
    size_t columns = il->columns;
    size_t line = il->line;

    if (saved_il == NULL)
	return;

    il_free(il);
    il = saved_il;

    /* We need this in case we got a SIGWINCH.  */
    il_resize(columns, line);
}


/*
 * Return the position of the point (relative to the beginning of the
 * dynamic section.
 */
size_t
il_point()
{
    return il->point - il->static_length;
}


/*
 * Set the echo state.  Returns the previous state of the echo flag.
 */
int
il_echo(echo)
    int echo;
{
    int old_echo_flag = il->echo;

    il->echo = echo;
    return old_echo_flag;
}


/*
 * Check if the input line is empty.
 */
int
il_is_empty()
{
    return il->dynamic_length == 0;
}


/*
 * Set mark where point is.
 */
void
il_set_mark()
{
    il->mark = il->point;
}


/*
 * Kill between point and mark.  The text is deleted but saved in the
 * kill ring.
 */
void
il_kill_region()
{
    il_region_command(IL_STORE | IL_KILL);
    il->last_operation = IL_KILL_REGION;
}


/*
 * Save the region as if killed, but don't kill it.
 */
void
il_kill_ring_save()
{
    il_region_command(IL_STORE);
    il->last_operation = IL_KILL_RING_SAVE;
}


/*
 * Reinsert the last stretch of killed text.
 */
void
il_yank()
{
    if (il->kill_ring)
    {
	il_insert_text(il->kill_ring);
	il->last_operation = IL_YANK;
    }
}


/*
 * Exchange point and mark.
 */
void
il_exchange_point_and_mark()
{
    int point = il->point;
    il->point = il->mark;
    il->mark  = point;
}


/*
 * Move the point backward one character.
 */
void
il_backward_char()
{
    if (il->point > il->static_length)
    {
	il->point--;
	il->last_operation = IL_BACKWARD_CHAR;
    }
}


/*
 * Move the point forward one character.
 */
void
il_forward_char()
{
    if (il->point < il->length)
    {
	il->point++;
	il->last_operation = IL_FORWARD_CHAR;
    }
}


/*
 * Move the point backward one word.
 */
void
il_backward_word()
{
    if (il->point > il->static_length)
    {
	while (il->point > il->static_length &&
	       il_separator(il->buffer[il->point - 1]))
	    il_backward_char();

	while (il->point > il->static_length &&
	       !il_separator(il->buffer[il->point - 1]))
	    il_backward_char();

	il->last_operation = IL_BACKWARD_WORD;
    }
}


/*
 * Move the point forward one word.
 */
void
il_forward_word()
{
    if (il->point < il->length)
    {
	while (il->point < il->length &&
	       il_separator(il->buffer[il->point]))
	    il_forward_char();

	while (il->point < il->length &&
	       !il_separator(il->buffer[il->point]))
	    il_forward_char();

	il->last_operation = IL_FORWARD_WORD;
    }
}


/*
 * Move the point at the beginning of the line.
 */
void
il_beginning_of_line()
{
    il->point = il->static_length;

    il->last_operation = IL_BEGINNING_OF_LINE;
}


/*
 * Move the point at the end of the line.
 */
void
il_end_of_line()
{
    il->point = il->length;

    il->last_operation = IL_END_OF_LINE;
}


/*
 * Insert 'c' at the point position.
 */
void
il_insert_char(c)
    int c;
{
    if (!isprint(c))
	return;

    if (il->length + 1 >= il->size)
	IL_RESIZE(il->length + 1 + 32);

    memmove(il->buffer + il->point + 1,
	    il->buffer + il->point,
	    il->length - il->point + 1);
    il->buffer[il->point] = c;
    il->point++;
    il->length++;
    il->dynamic_length++;

    il->last_operation = IL_INSERT_CHAR;
}


/*
 * Delete the character at the point.
 */
void
il_delete_char()
{
    if (il->point < il->length)
    {
	memcpy(il->buffer + il->point,
	       il->buffer + il->point + 1,
	       il->length - il->point + 1);

	il->length--;
	il->dynamic_length--;

	if (il->length % 16 == 0)
	    IL_RESIZE(il->length + 1);

	il->last_operation = IL_DELETE_CHAR;
    }
}


/*
 * Delete the character before the point.
 */
void
il_backward_delete_char()
{
    if (il->point > il->static_length)
    {
	memcpy(il->buffer + il->point - 1,
	       il->buffer + il->point,
	       il->length - il->point + 1);

	il->point--;
	il->length--;
	il->dynamic_length--;

	if (il->length % 16 == 0)
	    IL_RESIZE(il->length + 1);

	il->last_operation = IL_BACKWARD_DELETE_CHAR;
    }
}


/*
 * Kill characters forward until encountering the end of a word.
 */
void
il_kill_word()
{
    size_t end_of_word;
    size_t old_mark = il->mark;

    il_set_mark();
    il_forward_word();

    end_of_word = il->point;

    il_region_command(IL_STORE | IL_KILL);

    if (old_mark <= il->point)
	il->mark = old_mark;
    else
	if (old_mark >= end_of_word)
	    il->mark = old_mark - (end_of_word - il->point);
	else
	    il->mark = il->point;

    il->last_operation = IL_KILL_WORD;
}


/*
 * Kill characters backward until encountering the end of a word.
 */
void
il_backward_kill_word()
{
    size_t old_mark  = il->mark;
    size_t old_point = il->point;

    il_set_mark();
    il_backward_word();
    il_region_command(IL_STORE | IL_KILL);

    if (old_mark <= il->point)
	il->mark = old_mark;
    else
	if (old_mark >= old_point)
	    il->mark = old_mark - (old_point - il->point);
	else
	    il->mark = il->point;

    il->last_operation = IL_BACKWARD_KILL_WORD;
}


/*
 * Delete the entire line (both the static & dynamic parts).
 */
void
il_reset_line()
{
    il->point          = 0;
    il->mark           = 0;
    il->length         = 0;
    il->static_length  = 0;
    il->dynamic_length = 0;

    IL_RESIZE(1);

    il->buffer[0] = '\0';

    il->last_operation = IL_RESET_LINE;
}


/*
 * Delete the entire dynamic part of the line.
 */
void
il_kill_line(store)
    int store;
{
    il_beginning_of_line();
    il_set_mark();
    il_end_of_line();
    il_region_command(store | IL_KILL);
    il->mark = il->point;

    il->last_operation = IL_KILL_LINE;
}


/*
 * Delete the text between the beginning of the line and the point.
 */
void
il_kill_to_beginning_of_line()
{
    size_t old_mark = (il->mark <= il->point) ? il->static_length :
						il->mark - il->point;

    il_set_mark();
    il_beginning_of_line();
    il_region_command(IL_STORE | IL_KILL);
    il->mark = min(old_mark, il->length);

    il->last_operation = IL_KILL_TO_BEGINNING_OF_LINE;
}


/*
 * Delete the text between the point and the end of line.
 */
void
il_kill_to_end_of_line()
{
    size_t old_mark = il->mark;

    il_set_mark();
    il_end_of_line();
    il_region_command(IL_STORE | IL_KILL);
    il->mark = min(old_mark, il->length);

    il->last_operation = IL_KILL_TO_END_OF_LINE;
}


/*
 * Remove all the tabs and spaces arround the point, leaving just one
 * space.
 */
void
il_just_one_space()
{
    if (il->buffer[il->point] == ' ')
    {
	il_delete_horizontal_space();
	il_insert_char(' ');
	il->last_operation = IL_JUST_ONE_SPACE;
    }
}


/*
 * Remove all the tabs and spaces arround the point.
 */
void
il_delete_horizontal_space()
{
    if (il->buffer[il->point] == ' ')
    {
	while (il->buffer[il->point] == ' ')
	    il_delete_char();

	while (il->dynamic_length && il->buffer[il->point - 1] == ' ')
	    il_backward_delete_char();

	il->last_operation = IL_DELETE_HORIZONTAL_SPACE;
    }
}


/*
 * Convert the following word to lower case, moving over.
 */
void
il_downcase_word()
{
    if (il->point < il->length)
    {
	size_t i;
	size_t previous_point = il->point;

	/* Move to the end of the word.  */
	il_forward_word();

	for (i = previous_point; i < il->point; i++)
	    il->buffer[i] = tolower((int)il->buffer[i]);

	il->last_operation = IL_DOWNCASE_WORD;
    }
}


/*
 * Convert the following word to upper case, moving over.
 */
void
il_upcase_word()
{
    if (il->point < il->length)
    {
	size_t i;
	size_t previous_point = il->point;

	/* Move to the end of the word.  */
	il_forward_word();

	for (i = previous_point; i < il->point; i++)
	    il->buffer[i] = toupper((int)il->buffer[i]);

	il->last_operation = IL_UPCASE_WORD;
    }
}


/*
 * Capitalize the following word, moving over.  This gives the word
 * a first character in upper case and the rest lower case.
 */
void
il_capitalize_word()
{
    if (il->point < il->length)
    {
	size_t i;
	int first = 1;
	size_t previous_point = il->point;

	/* Move to the end of the word.  */
	il_forward_word();

	for (i = previous_point; i < il->point; i++)
	    if (isalnum((int)il->buffer[i]))
	    {
		if (first)
		{
		    il->buffer[i] = toupper((int)il->buffer[i]);
		    first = 0;
		}
		else
		    il->buffer[i] = tolower((int)il->buffer[i]);
	    }

	il->last_operation = IL_CAPITALIZE_WORD;
    }
}


/*
 * Set up the static text at the beginning of the input line.  This
 * text cannot be modified by normal editing commands.
 */
void
il_set_static_text(static_text)
    char *static_text;
{
    size_t len;

    assert(static_text);

    len = strlen(static_text);

    il->point += len - il->static_length;
    il->mark  += len - il->static_length;

    if (len + il->dynamic_length + 1 > il->size)
	IL_RESIZE(len + il->dynamic_length + 1);

    memmove(il->buffer + len,
	    il->buffer + il->static_length,
	    il->dynamic_length + 1);

    memcpy(il->buffer, static_text, len);

    toprintable(il->buffer, len);

    il->length = (il->static_length = len) + il->dynamic_length;
    il->last_operation = IL_SET_STATIC_TEXT;
}


/*
 * Insert 'text' at the point position.
 */
void
il_insert_text(text)
    char *text;
{
    size_t len;

    if (text == NULL)
	return;

    len = strlen(text);

    if (il->length + len + 1 > il->size)
	IL_RESIZE(il->size + len + 1 + 32);

    memmove(il->buffer + il->point + len,
	    il->buffer + il->point,
	    il->length - il->point + 1);

    memcpy(il->buffer + il->point, text, len);

    toprintable(il->buffer + il->point, len);

    il->point          += len;
    il->length         += len;
    il->dynamic_length += len;

    il->last_operation = IL_INSERT_TEXT;
}


/*
 * Temporary hide the static part of the input line.  This is only used
 * when the visible part of the input line becomes smaller than the
 * static size.
 */
static int
il_hide_static()
{
    int normal_static_length = il->static_length;

    il->buffer += il->static_length;
    il->length -= il->static_length;
    il->point -= il->static_length;
    il->mark -= il->static_length;
    il->static_length = 0;

    return normal_static_length;
}


/*
 * Restore the static part of the input line.
 */
static void
il_restore_static(normal_static_length)
    int normal_static_length;
{
    il->static_length = normal_static_length;
    il->buffer -= il->static_length;
    il->length += il->static_length;
    il->point += il->static_length;
    il->mark += il->static_length;
}


/*
 * Update the point.
 */
void
il_update_point()
{
    int scroll;
    size_t len;
    size_t normal_static_length = 0;
    int il_too_small = il->columns < il->static_length + 3;

    tty_colors(il->error ? InputLineErrorBrightness : InputLineBrightness,
	       il->error ? InputLineErrorForeground : InputLineForeground,
	       il->error ? InputLineErrorBackground : InputLineBackground);

    if (il_too_small)
	normal_static_length = il_hide_static();

    scroll = il_compute_scroll();

    len = ((il->point >= il->columns) ?
	   il->point - il->columns + 1 +
	   (scroll - 1) - ((il->point - il->columns) % scroll) : 0);

    window_goto(il->window, 0, il->point - len);

    if (il_too_small)
	il_restore_static(normal_static_length);
}


/*
 * Update the entire input line.
 */
void
il_update()
{
    int scroll;
    char *temp;
    unsigned len;
    tty_status_t status;
    size_t normal_static_length = 0;
    int il_too_small = il->columns < il->static_length + 3;

    tty_save(&status);

    tty_colors(il->error ? InputLineErrorBrightness : InputLineBrightness,
	       il->error ? InputLineErrorForeground : InputLineForeground,
	       il->error ? InputLineErrorBackground : InputLineBackground);

    if (il_too_small)
	normal_static_length = il_hide_static();

    scroll = il_compute_scroll();

    len = ((il->point >= il->columns) ?
	   il->point - il->columns + 1 +
	   (scroll - 1) - ((il->point - il->columns) % scroll) : 0);

    temp = xmalloc(il->columns);
    memset(temp, ' ', il->columns);

    if (il->echo)
	memcpy(temp, il->buffer + il->static_length + len,
	       min(il->length   - il->static_length - len,
		   il->columns  - il->static_length));
    else
	memset(temp, '*',
	       min(il->length   - il->static_length - len,
		   il->columns  - il->static_length));

    window_goto(il->window, 0, 0);

    if (!il_too_small)
	window_puts(il->window, il->buffer, il->static_length);

    window_puts(il->window, temp, il->columns - il->static_length);

    /* If we don't do this, the screen cursor will annoyingly jump to
       the left margin of the command line.  */
    window_goto(il->window, 0, il->point - len);

    if (il_too_small)
	il_restore_static(normal_static_length);

    xfree(temp);
    tty_restore(&status);
}


/*
 * Get the input line contents (only the dynamic part).
 * dest is a pointer to a NULL pointer or a pointer to a pointer allocated
 * with xmalloc().
 */
int
il_get_contents(dest)
    char **dest;
{
    *dest = xrealloc(*dest, il->dynamic_length + 1);
    memcpy(*dest, il->buffer + il->static_length, il->dynamic_length + 1);
    return il->dynamic_length;
}


/*
 * Display a message into the input line.
 */
void
il_message(message)
    char *message;
{
    il_reset_line();
    il_set_static_text(message ? message : "Wait....");
    il_update();
    il_update_point();
}


void
il_set_error_flag(flag)
    int flag;
{
    il->error = flag;
}


/*
 * inputline's interface to the history library.
 */
void
il_history(dir)
    int dir;
{
    static int browsing = 0;
    static int last_history_position;
    HIST_ENTRY *hist;

    switch (dir)
    {
	case IL_PREVIOUS:
	    if (!browsing)
	    {
		browsing = 1;
		last_history_position = where_history();
	    }

	    if ((hist = previous_history()))
	    {
		il->dynamic_length = strlen(hist->line);
		il->length         = il->static_length + il->dynamic_length;
		il->point          = il->length;

		if (il->length + 1 > il->size)
		    IL_RESIZE(il->length + 1);

		strcpy(il->buffer + il->static_length, hist->line);

		il_update();
	    }

	    break;

	case IL_NEXT:
	    if (!browsing)
	    {
		browsing = 1;
		last_history_position = where_history();
	    }

	    if ((hist = next_history()))
	    {
		il->dynamic_length = strlen(hist->line);
		il->length         = il->static_length + il->dynamic_length;
		il->point          = il->length;

		if (il->length + 1 > il->size)
		    IL_RESIZE(il->length + 1);

		strcpy(il->buffer + il->static_length, hist->line);
	    }
	    else
		il_kill_line(IL_DONT_STORE);

	    il_update();
	    break;

	case IL_RECORD:
	    if (browsing)
	    {
		history_set_pos(last_history_position);
		browsing = 0;
	    }

	    if ((hist = previous_history()))
	    {
		if (strcmp(il->buffer + il->static_length, hist->line) != 0)
		{
		    add_history(il->buffer + il->static_length);
		    next_history();
		}
	    }
	    else
		add_history(il->buffer + il->static_length);

	    next_history();
	    break;

	default:
	    break;
    }
}
