/* gitview.c -- A hex/ascii file viewer.  */

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
/* Patched by Gregory Gerard <ggerard@ssl.sel.sony.com> to display on
   the entire screen.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else /* !HAVE_STDLIB_H */
#include "ansi_stdlib.h"
#endif /* !HAVE_STDLIB_H */

#include <sys/types.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <limits.h>
#include "file.h"
#include <signal.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <assert.h>

#include "stdc.h"
#include "xstring.h"
#include "xmalloc.h"
#include "getopt.h"
#include "xio.h"
#include "window.h"
#include "signals.h"
#include "configure.h"
#include "tty.h"
#include "misc.h"
#include "tilde.h"


#define MAX_KEYS                        2048

#define BUILTIN_OPERATIONS              11


#define BUILTIN_previous_line           -1
#define BUILTIN_next_line               -2
#define BUILTIN_scroll_down             -3
#define BUILTIN_scroll_up               -4
#define BUILTIN_beginning_of_file       -5
#define BUILTIN_end_of_file             -6
#define BUILTIN_refresh                 -7
#define BUILTIN_exit                    -8
#define BUILTIN_hard_refresh            -9
#define BUILTIN_backspace               -10
#define BUILTIN_action                  -11


#define MAX_BUILTIN_NAME                20

char built_in[BUILTIN_OPERATIONS][MAX_BUILTIN_NAME] =
{
    "previous-line",
    "next-line",
    "scroll-down",
    "scroll-up",
    "beginning-of-file",
    "end-of-file",
    "refresh",
    "exit",
    "hard-refresh",
    "backspace",
    "action",
};


#define VIEWER_FIELDS 12

static char *ViewerFields[VIEWER_FIELDS] =
{
    "TitleForeground",
    "TitleBackground",
    "TitleBrightness",
    "HeaderForeground",
    "HeaderBackground",
    "HeaderBrightness",
    "ScreenForeground",
    "ScreenBackground",
    "ScreenBrightness",
    "StatusForeground",
    "StatusBackground",
    "StatusBrightness"
};

#ifdef HAVE_LINUX
static int ViewerColors[VIEWER_FIELDS] =
{
    CYAN, BLUE, ON, CYAN, RED, ON, BLACK, CYAN, OFF, CYAN, BLUE, ON
};
#else   /* !HAVE_LINUX */
static int ViewerColors[VIEWER_FIELDS] =
{
    BLACK, WHITE, OFF, WHITE, BLACK, ON, WHITE, BLACK, OFF, BLACK, WHITE, OFF
};
#endif  /* !HAVE_LINUX */

#define TitleForeground                 ViewerColors[0]
#define TitleBackground                 ViewerColors[1]
#define TitleBrightness                 ViewerColors[2]
#define HeaderForeground                ViewerColors[3]
#define HeaderBackground                ViewerColors[4]
#define HeaderBrightness                ViewerColors[5]
#define ScreenForeground                ViewerColors[6]
#define ScreenBackground                ViewerColors[7]
#define ScreenBrightness                ViewerColors[8]
#define StatusForeground                ViewerColors[9]
#define StatusBackground                ViewerColors[10]
#define StatusBrightness                ViewerColors[11]


#ifdef HAVE_LINUX
extern int LinuxConsole;
#endif /* HAVE_LINUX */

#ifdef HAVE_LINUX
int AnsiColors = ON;
#else   /* !HAVE_LINUX */
int AnsiColors = OFF;
#endif  /* !HAVE_LINUX */


#define VIEW_LINES (tty_lines - 9)
#define SEEK_LINE (tty_lines - 5)


#ifdef STAT_MACROS_BROKEN
#ifdef S_IFREG
#undef S_IFREG
#endif
#ifdef S_IFBLK
#undef S_IFBLK
#endif
#endif /* STAT_MACROS_BROKEN */


#ifndef S_IFREG
#define S_IFREG         0100000
#endif

#ifndef S_IFBLK
#define S_IFBLK         0060000
#endif

#ifndef S_ISREG
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISBLK
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
#endif

/* Finally ... :-( */


char *g_home;
char *g_program;
/* for gnulib lib/error.c */
char *program_name;
char *screen;
char *filename;
int count;
off64_t g_size;
char g_offset[16];
char *header_text;
int  UseLastScreenChar;
char *global_buf;
char color_section[]  = "[GITVIEW-Color]";
char monochrome_section[] = "[GITVIEW-Monochrome]";
int fd, regular_file;
long long g_current_line, g_lines;
window_t *title_window, *header_window, *file_window, *status_window;
static char *title_text;
static char *g_help;
static char info_txt[] =
    "   Offset     00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F       \
Ascii       ";
static char line_txt[]    =
    "---------------------------------------------------------------------\
----------- ";
static char seek_txt[]    = " Seek at: ";


off64_t
file_length()
{
    off64_t current, length;

    if (!regular_file)
	return 0x7FFFFFFF;

    current = lseek64(fd, 0, SEEK_CUR);
    length  = lseek64(fd, 0, SEEK_END);
    lseek64(fd, current, SEEK_SET);
    return length;
}


void
cursor_update()
{
    if (tty_lines >= 9)
	window_goto(file_window, SEEK_LINE, strlen(seek_txt) + count);
    else
	window_goto(file_window, tty_lines - 1, tty_columns - 1);
}


void
set_title()
{
    memset(global_buf, ' ', tty_columns);
    memcpy(global_buf, title_text,
	   min(tty_columns, (int)strlen(title_text)));

    tty_colors(TitleBrightness, TitleForeground, TitleBackground);

    window_goto(title_window, 0, 0);
    window_puts(title_window, global_buf, tty_columns);
}


void
set_header()
{
    memset(global_buf, ' ', tty_columns);
    memcpy(global_buf, header_text,
	   min(tty_columns, (int)strlen(header_text)));

    tty_colors(HeaderBrightness, HeaderForeground, HeaderBackground);

    window_goto(header_window, 0, 0);
    window_puts(header_window, global_buf, tty_columns);
}


void
set_status()
{
    memset(global_buf, ' ', tty_columns);
    memcpy(global_buf, g_help, min(tty_columns, (int)strlen(g_help)));

    tty_colors(StatusBrightness, StatusForeground, StatusBackground);

    window_goto(status_window, 0, 0);
    window_puts(status_window, global_buf, tty_columns);
}


void
report_undefined_key()
{
    char *prev = tty_get_previous_key_seq();
    size_t length = strlen(prev);

    if (length && (prev[length - 1] != key_INTERRUPT))
    {
	char *str = (char *)tty_key_machine2human(tty_get_previous_key_seq());
	char *buf = xmalloc(128 + strlen(str));
	sprintf(buf, "%s: not defined.", str);
	memset(global_buf, ' ', tty_columns);
	memcpy(global_buf, buf, min(tty_columns, (int)strlen(buf)));
	xfree(buf);

	tty_colors(ON, WHITE, RED);
	window_goto(status_window, 0, 0);
	window_puts(status_window, global_buf, tty_columns);

	tty_beep();
	tty_update();
	sleep(1);
    }
    else
	tty_beep();

    set_status();
    cursor_update();
    tty_update();
}


char
char_to_print(c, index, total)
    char c;
    int index;
    int total;
{
    if (index < total)
	return isprint((int)c) ? c : '.';
    return ' ';
}


void
update_line(line)
    long long line;
{
    ssize_t r;
    unsigned char buf[16];
    char *line_string = xmalloc(max(tty_columns, 80 + 1));

    memset(line_string, ' ', tty_columns);
    memset(buf, '\0', sizeof(buf));
    lseek64(fd, (off64_t)line * sizeof(buf), SEEK_SET);

    if ((r = read(fd, buf, sizeof(buf))))
    {
	sprintf(line_string, "%011X0  %02X %02X %02X %02X %02X %02X %02X\
 %02X  %02X %02X %02X %02X %02X %02X %02X %02X  \
%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c ",
		(unsigned int)(line & 0xFFFFFFFF),
		buf[0], buf[1], buf[2], buf[3],
		buf[4], buf[5], buf[6], buf[7],
		buf[8], buf[9], buf[10], buf[11],
		buf[12], buf[13], buf[14], buf[15],
		char_to_print(buf[0], 0, r), char_to_print(buf[1], 1, r),
		char_to_print(buf[2], 2, r), char_to_print(buf[3], 3, r),
		char_to_print(buf[4], 4, r), char_to_print(buf[5], 5, r),
		char_to_print(buf[6], 6, r), char_to_print(buf[7], 7, r),
		char_to_print(buf[8], 8, r), char_to_print(buf[9], 9, r),
		char_to_print(buf[10], 10, r), char_to_print(buf[11], 11, r),
		char_to_print(buf[12], 12, r), char_to_print(buf[13], 13, r),
		char_to_print(buf[14], 14, r), char_to_print(buf[15], 15, r));
    }
    else
    {
	r = 0;
	*line_string = '\0';
    }

    if (r < 8)
	memset(line_string + 12 + r * 3, ' ', (16 - r) * 3 + 1);
    else if (r >= 8 && r < 16)
	memset(line_string + 12 + r * 3 + 1, ' ', (16 - r) * 3);

    line_string[strlen(line_string)] = ' ';
    window_puts(file_window, line_string, tty_columns);
    xfree(line_string);
}


void
update_all()
{
    long long i;

    tty_colors(ScreenBrightness, ScreenForeground, ScreenBackground);

    for (i = g_current_line; i < g_current_line + VIEW_LINES; i++)
    {
	window_goto(file_window, 3 + i - g_current_line, 0);
	update_line(i);
    }
}


void
clean_up()
{
    tty_end(NULL);
}


void
fatal(postmsg)
    char *postmsg;
{
    clean_up();
    fprintf(stderr, "%s: fatal error: %s.\n", g_program, postmsg);
    exit(1);
}


int
read_keys(keys)
    int keys;
{
    char *contents;
    char key_seq[80];
    int i, j, need_conversion;


    for (i = keys; i < MAX_KEYS; i++)
    {
	configuration_getvarinfo(key_seq, &contents, 1, NO_SEEK);

	if (*key_seq == 0)
	    break;

	if (*key_seq != '^')
	{
	    char *key_seq_ptr = tty_get_symbol_key_seq(key_seq);

	    if (key_seq_ptr)
	    {
		/* Ignore empty/invalid key sequences.  */
		if (*key_seq_ptr == '\0')
		    continue;

		/* We got the key sequence in the correct form, as
		   returned by tgetstr, so there is no need for
		   further conversion.  */
		strcpy(key_seq, key_seq_ptr);
		need_conversion = 0;
	    }
	    else
	    {
		/* This is not a TERMCAP symbol, it is a key sequence
		   that we will have to convert it with
		   tty_key_human2machine() into a machine usable form
		   before using it.  */
		need_conversion = 1;
	    }
	}
	else
	    need_conversion = 1;

	if (contents == NULL)
	    continue;

	for (j = 0; j < BUILTIN_OPERATIONS; j++)
	    if (strcmp(contents, built_in[j]) == 0)
		break;

	if (j < BUILTIN_OPERATIONS)
	{
	    if (!need_conversion ||
		tty_key_human2machine((unsigned char *)key_seq))
		tty_key_list_insert((unsigned char *)key_seq,  built_in[-j-1]);
	}
	else
	    fprintf(stderr, "%s: invalid built-in operation: %s.\n",
		    g_program, contents);
    }

    return i;
}


void
resize(resize_required)
    int resize_required;
{
    int display_status = OFF;
    int display_header = OFF;
    int display_file = OFF;
    int old_tty_lines = tty_lines;
    int old_tty_columns = tty_columns;

    tty_resize();

    /* Don't resize, unless absolutely necessary.  */
    if (!resize_required)
	if (tty_lines == old_tty_lines && tty_columns == old_tty_columns)
	    return;

#ifdef HAVE_LINUX
    if (LinuxConsole)
	screen = xrealloc(screen, 4 + tty_columns * tty_lines * 2);
#endif  /* HAVE_LINUX */

    /* Watch out for special cases (tty_lines < 7) because some
     * components will no longer fit.
     *
     * The title needs one line.  The panels need four.  There is one
     * more line for the input line and one for the status.
     *
     * Cases (lines available):
     *      1 line:	title
     *      2 lines:	title + status
     *      3 lines:	title + header + status
     *	 >= 4 lines:	everything
     *
     * FIXME: we need to handle the tty_columns case.
     */

    global_buf  = xrealloc(global_buf, tty_columns + 1);
    header_text = xrealloc(header_text, strlen(filename) + 10);

    window_resize(title_window, 0, 0, 1, tty_columns);

    if (tty_lines >= 2)
	display_status = ON;

    if (tty_lines >= 3)
	display_header = ON;

    if (tty_lines >= 4)
	display_file = ON;

    window_resize(status_window, 0, tty_lines - 1,
		  display_status ? 1 : 0, tty_columns);
    window_resize(header_window, 0, 1,
		  display_header ? 1 : 0, tty_columns);
    window_resize(file_window, 0, 2,
		  display_file ? (tty_lines - 2) : 0, tty_columns);
}


/*
 * Resize (if necessary) and then refresh all gitview's components.
 */

void
refresh(signum)
    int signum;
{
    resize(0);

    if (signum == SIGCONT)
    {
	/* We were suspended.  Switch back to noncanonical mode.  */
	tty_set_mode(TTY_NONCANONIC);
	tty_defaults();
    }

    tty_colors(ScreenBrightness, ScreenForeground, ScreenBackground);
    tty_fill();

    g_size = file_length();
    g_lines = g_size / 16 + (g_size % 16 ? 1 : 0);

    if (tty_lines >= 5)
    {
	window_goto(file_window, 1, 0);
	window_puts(file_window, info_txt, sizeof(info_txt) - 1);
    }

    if (tty_lines >= 6)
    {
	window_goto(file_window, 2, 0);
	window_puts(file_window, line_txt, sizeof(line_txt) - 1);
    }

    if (tty_lines >= 9)
    {
	if (VIEW_LINES == 0)
	    g_current_line = 0;
	else
	    g_current_line = min(g_current_line, (g_lines / VIEW_LINES) * VIEW_LINES);

	window_goto(file_window, SEEK_LINE, 0);
	window_puts(file_window, seek_txt, sizeof(seek_txt) - 1);
	window_goto(file_window, SEEK_LINE, sizeof(seek_txt) - 1);
	window_puts(file_window, g_offset, count);
    }
    else
	g_current_line = 0;

    set_title();
    set_status();
    set_header();

    update_all();

    if (tty_lines >= 9)
	window_goto(file_window, SEEK_LINE, sizeof(seek_txt) - 1 + count);
    else
	window_goto(file_window, tty_lines - 1, tty_columns - 1);

    tty_update();

    if (signum == SIGCONT)
	tty_update_title(header_text);
}


void
hide()
{
    tty_set_mode(TTY_CANONIC);
    tty_defaults();
    tty_put_screen(screen);
}


void
clock_refresh()
{
}


void
usage()
{
    printf("usage: %s [-hvicbl] file\n", g_program);
    printf(" -h         print this help message\n");
    printf(" -v         print the version number\n");
    printf(" -c         use ANSI colors\n");
    printf(" -b         don't use ANSI colors\n");
    printf(" -l         don't use the last screen character\n");
}


int
main(argc, argv)
    int argc;
    char *argv[];
{
    int key = 0;
    struct stat s;
    tty_key_t *ks;
    int keys, repeat_count, need_update;
    int c, ansi_colors = -1, use_last_screen_character = ON;

#ifdef HAVE_SETLOCALE
    setlocale(LC_ALL,"");
#endif

    /* Make sure we don't get signals before we are ready to handle
       them.  */
    signals_init();

    program_name = g_program = argv[0];

    g_home = getenv("HOME");
    if (g_home == NULL)
	g_home = ".";

    compute_directories();
    get_login_name();

    if (getenv("COLORTERM") != NULL)
	ansi_colors = ON;

    /* Parse the command line.  */
    while ((c = getopt(argc, argv, "hvcblp")) != -1)
	switch (c)
	{
	    case 'h':
		/* Help request.  */
		usage();
		return 0;

	    case 'v':
		/* Version number request.  */
		printf("%s %s\n", PRODUCT, VERSION);
		return 0;

	    case 'c':
		/* Force git to use ANSI color sequences.  */
		ansi_colors = ON;
		break;

	    case 'b':
		/* Prevent git from using ANSI color sequences.  */
		ansi_colors = OFF;
		break;

	    case 'l':
		/* Prevent git from using the last character on the
		   screen.  */
		use_last_screen_character = OFF;
		break;

	    case '?':
		return 1;

	    default:
		fprintf(stderr, "%s: unknown error\n", g_program);
		return 1;
	}

    if (optind < argc)
	filename = xstrdup(argv[optind++]);
    else
    {
	usage();
	return 1;
    }

    if (optind < argc)
	fprintf(stderr, "%s: warning: invalid extra options ignored\n",
		g_program);

    title_text = xmalloc(strlen(PRODUCT) + strlen(VERSION) + 64);
    sprintf(title_text, " %s %s - Hex/Ascii File Viewer", PRODUCT, VERSION);

    tty_init(TTY_RESTRICTED_INPUT);

    xstat(filename, &s);

/*
    if (!(S_ISREG(s.st_mode) || S_ISBLK(s.st_mode)))
    {
	fprintf(stderr, "%s: %s is neither regular file nor block device.\n",
		g_program, filename);
	return 1;
    }
*/
    fd = open64(filename, O_RDONLY | O_BINARY);

    if (fd == -1)
    {
	fprintf(stderr, "%s: cannot open file %s.\n", g_program, filename);
	return 1;
    }

    regular_file = S_ISREG(s.st_mode);

    common_configuration_init();
    use_section("[GITVIEW-Keys]");
    keys = read_keys(0);
    configuration_end();

    specific_configuration_init();
    use_section("[Setup]");

    if (ansi_colors == -1)
	AnsiColors = get_flag_var("AnsiColors", OFF);
    else
	AnsiColors = ansi_colors;

    if (use_last_screen_character)
	UseLastScreenChar = get_flag_var("UseLastScreenChar",  OFF);
    else
	UseLastScreenChar = OFF;

    tty_set_last_char_flag(UseLastScreenChar);

    use_section("[GITVIEW-Setup]");
    g_help = get_string_var("Help", "");

    use_section(AnsiColors ? color_section : monochrome_section);
    get_colorset_var(ViewerColors, ViewerFields, VIEWER_FIELDS);

    use_section("[GITVIEW-Keys]");
    keys = read_keys(keys);

    if (keys == MAX_KEYS)
	fprintf(stderr, "%s: too many key sequences; only %d are allowed.\n",
		g_program, MAX_KEYS);

    configuration_end();

    tty_start_cursorapp();

    title_window  = window_init();
    header_window = window_init();
    file_window = window_init();
    status_window = window_init();

    resize(0);

    tty_get_screen(screen);
    tty_set_mode(TTY_NONCANONIC);
    tty_defaults();

    signal_handlers(ON);

    g_offset[count]  = 0;
    g_current_line = 0;

    sprintf(header_text, " File: %s", filename);
    tty_update_title(header_text);

  restart:
    refresh(0);

    while (1)
    {
	while ((ks = tty_get_key(&repeat_count)) == NULL)
	    report_undefined_key();
	set_status();

	if (ks->aux_data == NULL)
	    key = ks->key_seq[0];
	else
	    key = ((char *)ks->aux_data - (char *)built_in) / MAX_BUILTIN_NAME;

	g_size = file_length();
	g_lines = g_size / 16 + (g_size % 16 ? 1 : 0);

	switch (key)
	{
	    case BUILTIN_previous_line:
		need_update = 0;

		while (repeat_count--)
		{
		    if (g_current_line == 0)
			break;

		    g_current_line--, need_update = 1;
		}

		if (need_update)
		    update_all();

		break;

	    case BUILTIN_next_line:
		need_update = 0;

		while (repeat_count--)
		{
		    if (g_current_line >= g_lines - VIEW_LINES)
			break;

		    g_current_line++, need_update = 1;
		}

		if (need_update)
		    update_all();

		break;

	    case BUILTIN_backspace:
		if (count)
		{
		    count--;

		    if (tty_lines >= 9)
		    {
			tty_colors(ScreenBrightness,
				   ScreenForeground,
				   ScreenBackground);
			window_goto(file_window, SEEK_LINE,
				    strlen(seek_txt) + count);
			window_putc(file_window, ' ');
		    }
		    else
			window_goto(file_window,
				    tty_lines - 1, tty_columns - 1);
		    break;
		}

	    case BUILTIN_scroll_down:
		if (g_current_line == 0)
		    break;

		g_current_line = max(0, g_current_line - VIEW_LINES);
		update_all();
		break;

	    case ' ':
	    case BUILTIN_scroll_up:
		if (g_current_line >= g_lines - VIEW_LINES)
		    break;

		g_current_line = min(g_lines - 1, g_current_line + VIEW_LINES);
		update_all();
		break;

	    case BUILTIN_beginning_of_file:
		if (g_current_line)
		{
		    g_current_line = 0;
		    update_all();
		}

		break;

	    case BUILTIN_end_of_file:
		if (regular_file && g_current_line < g_lines - VIEW_LINES)
		{
		    g_current_line = g_lines - VIEW_LINES;
		    update_all();
		}

		break;

	    case BUILTIN_hard_refresh:
	    case BUILTIN_refresh:
		goto restart;

	    case '0': case '1': case '2': case '3': case '4': case '5':
	    case '6': case '7': case '8': case '9': case 'A': case 'B':
	    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
	    case 'c': case 'd': case 'e': case 'f':
		if (count < 8)
		{
		    if (tty_lines >= 9)
		    {
			char tmp;
			tty_colors(ScreenBrightness,
				   ScreenForeground,
				   ScreenBackground);
			window_goto(file_window, SEEK_LINE,
				    strlen(seek_txt) + count);
			tmp = (char)key;
			window_putc(file_window, tmp);
			g_offset[count++] = tmp;
		    }
		}
		else
		    tty_beep();
		break;

	    case BUILTIN_action:
		if (count == 0)
		    tty_beep();
		else
		{
		    if (tty_lines >= 9)
		    {
			g_offset[count] = 0;

			sscanf(g_offset, "%x", &count);
			tty_colors(ScreenBrightness,
				   ScreenForeground,
				   ScreenBackground);
			window_goto(file_window, SEEK_LINE, strlen(seek_txt));
			window_puts(file_window, "        ", 8);

			if (count < 0)
			    count = 0;

			if (count > g_size)
			    count = g_size;

			g_current_line = count >> 4;
			update_all();
			count = 0;
		    }
		}

		break;

	    case 'q':
	    case BUILTIN_exit:
		goto end;

	    default:
		report_undefined_key();
		break;
	}

	cursor_update();
    }

  end:
    tty_set_mode(TTY_CANONIC);
    tty_end_cursorapp();
    tty_end(screen);
    return 0;
}
