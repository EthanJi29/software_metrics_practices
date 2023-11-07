/* gitps.c -- A process viewer/killer utility.  */

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

#include <ctype.h>
#include <limits.h>
#include "file.h"
#include <fcntl.h>
#include <signal.h>
#include <time.h>

/* SVR2/SVR3.  */
#if !(defined(SIGCHLD)) && defined(SIGCLD)
#define SIGCHLD	SIGCLD
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <errno.h>

/* Not all systems declare ERRNO in errno.h... and some systems #define it! */
#if !defined (errno)
extern int errno;
#endif /* !errno */

#include <assert.h>

#include "stdc.h"
#include "xstring.h"
#include "xmalloc.h"
#include "getopt.h"
#include "tty.h"
#include "window.h"
#include "configure.h"
#include "tilde.h"
#include "signals.h"
#include "misc.h"


#define MAX_KEYS        2048
#define MAX_LINE	2048
#define PS_FIELDS         12


static char *PSFields[PS_FIELDS] =
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
static int PSColors[PS_FIELDS] =
{
    CYAN, BLUE, ON, CYAN, RED, ON, BLACK, CYAN, OFF, CYAN, BLUE, ON
};
#else   /* !HAVE_LINUX */
static int PSColors[PS_FIELDS] =
{
    BLACK, WHITE, OFF, WHITE, BLACK, ON, WHITE, BLACK, OFF, BLACK, WHITE, OFF
};
#endif  /* !HAVE_LINUX */

#define TitleForeground                 PSColors[0]
#define TitleBackground                 PSColors[1]
#define TitleBrightness                 PSColors[2]
#define HeaderForeground                PSColors[3]
#define HeaderBackground                PSColors[4]
#define HeaderBrightness                PSColors[5]
#define ScreenForeground                PSColors[6]
#define ScreenBackground                PSColors[7]
#define ScreenBrightness                PSColors[8]
#define StatusForeground                PSColors[9]
#define StatusBackground                PSColors[10]
#define StatusBrightness                PSColors[11]


#ifdef HAVE_LINUX
extern int LinuxConsole;
#endif /* HAVE_LINUX */

#ifdef HAVE_LINUX
int AnsiColors = ON;
#else   /* !HAVE_LINUX */
int AnsiColors = OFF;
#endif  /* !HAVE_LINUX */


char color_section[]  = "[GITPS-Color]";
char monochrome_section[] = "[GITPS-Monochrome]";
int  processes;
int  PID_index;

char *g_home;
char *g_program;
/* for gnulib lib/error.c */
char *program_name;
char *ps_cmd;
char *temporary_directory;
char header_text[MAX_LINE];
int UseLastScreenChar;
int StartupScrollStep;
int RefreshAfterKill;
char *stdout_log_name;
char *stderr_log_name;
char *stdout_log_template;
char *stderr_log_template;
char **ps_vect;
char *screen;
char *global_buf;
int first_on_screen, current_process, scroll_step;
static int horizontal_offset=0;
window_t *title_window, *header_window, *processes_window, *status_window;
static char *title_text;
static char *help;
static char no_perm[] = "not owner !";
static char no_proc[] = "no such process ! (REFRESH recommended)";


typedef struct
{
    char signame[10];
    int signal;
} xsignal_t;

int signal_type = 0;

static xsignal_t sigdesc[] =
{
    { "SIGALRM  ", SIGALRM   },
    { "SIGABRT  ", SIGABRT   },
#ifdef SIGBUS
    { "SIGBUS   ", SIGBUS    },
#endif
#ifdef SIGCHLD
    { "SIGCHLD  ", SIGCHLD   },
#endif
#ifdef SIGCLD
    { "SIGCLD   ", SIGCLD    },
#endif
#ifdef SIGCONT
    { "SIGCONT  ", SIGCONT   },
#endif
#ifdef SIGEMT
    { "SIGEMT   ", SIGEMT    },
#endif
#ifdef SIGFPE
    { "SIGFPE   ", SIGFPE    },
#endif
    { "SIGHUP   ", SIGHUP    },
#ifdef SIGILL
    { "SIGILL   ", SIGILL    },
#endif
#ifdef SIGINFO
    { "SIGINFO  ", SIGINFO   },
#endif
    { "SIGINT   ", SIGINT    },
#ifdef SIGIO
    { "SIGIO    ", SIGIO     },
#endif
#ifdef SIGIOT
    { "SIGIOT   ", SIGIOT    },
#endif
    { "SIGKILL  ", SIGKILL   },
    { "SIGPIPE  ", SIGPIPE   },
#ifdef SIGPOLL
    { "SIGPOLL  ", SIGPOLL   },
#endif
#ifdef SIGPROF
    { "SIGPROF  ", SIGPROF   },
#endif
#ifdef SIGPWR
    { "SIGPWR   ", SIGPWR    },
#endif
    { "SIGQUIT  ", SIGQUIT   },
    { "SIGSEGV  ", SIGSEGV   },
#ifdef SIGSTOP
    { "SIGSTOP  ", SIGSTOP   },
#endif
#ifdef SIGSYS
    { "SIGSYS   ", SIGSYS    },
#endif
    { "SIGTERM  ", SIGTERM   },
#ifdef SIGSTKFLT
    { "SIGSTKFLT", SIGSTKFLT },
#endif
#ifdef SIGTRAP
    { "SIGTRAP  ", SIGTRAP   },
#endif
#ifdef SIGTSTP
    { "SIGTSTP  ", SIGTSTP   },
#endif
#ifdef SIGTTIN
    { "SIGTTIN  ", SIGTTIN   },
#endif
#ifdef SIGTTOU
    { "SIGTTOU  ", SIGTTOU   },
#endif
#ifdef SIGURG
    { "SIGURG   ", SIGURG    },
#endif
    { "SIGUSR1  ", SIGUSR1   },
    { "SIGUSR2  ", SIGUSR2   },
#ifdef SIGVTALRM
    { "SIGVTALRM", SIGVTALRM },
#endif
#ifdef SIGWINCH
    { "SIGWINCH ", SIGWINCH  },
#endif
#ifdef SIGXCPU
    { "SIGXCPU  ", SIGXCPU   },
#endif
#ifdef SIGXFSZ
    { "SIGXFSZ  ", SIGXFSZ   },
#endif
};


#define BUILTIN_OPERATIONS              49


#define BUILTIN_previous_line            0
#define BUILTIN_next_line                1
#define BUILTIN_scroll_down              2
#define BUILTIN_scroll_up                3
#define BUILTIN_beginning_of_list        4
#define BUILTIN_end_of_list              5
#define BUILTIN_next_signal              6
#define BUILTIN_SIGALRM                  7
#define BUILTIN_SIGABRT                  8
#define BUILTIN_SIGBUS                   9
#define BUILTIN_SIGCHLD                 10
#define BUILTIN_SIGCLD                  11
#define BUILTIN_SIGCONT                 12
#define BUILTIN_SIGEMT                  13
#define BUILTIN_SIGFPE                  14
#define BUILTIN_SIGHUP                  15
#define BUILTIN_SIGILL                  16
#define BUILTIN_SIGINFO                 17
#define BUILTIN_SIGINT                  18
#define BUILTIN_SIGIO                   19
#define BUILTIN_SIGIOT                  20
#define BUILTIN_SIGKILL                 21
#define BUILTIN_SIGPIPE                 22
#define BUILTIN_SIGPOLL                 23
#define BUILTIN_SIGPROF                 24
#define BUILTIN_SIGPWR                  25
#define BUILTIN_SIGQUIT                 26
#define BUILTIN_SIGSEGV                 27
#define BUILTIN_SIGSTOP                 28
#define BUILTIN_SIGSYS                  29
#define BUILTIN_SIGTERM                 30
#define BUILTIN_SIGTRAP                 31
#define BUILTIN_SIGTSTP                 32
#define BUILTIN_SIGTTIN                 33
#define BUILTIN_SIGTTOU                 34
#define BUILTIN_SIGURG                  35
#define BUILTIN_SIGUSR1                 36
#define BUILTIN_SIGUSR2                 37
#define BUILTIN_SIGVTALRM               38
#define BUILTIN_SIGWINCH                39
#define BUILTIN_SIGXCPU                 40
#define BUILTIN_SIGXFSZ                 41
#define BUILTIN_kill_process            42
#define BUILTIN_refresh                 43
#define BUILTIN_exit                    44
#define BUILTIN_hard_refresh            45
#define BUILTIN_SIGSTKFLT               46
#define BUILTIN_horizontal_scroll_left  47
#define BUILTIN_horizontal_scroll_right 48

#define MAX_BUILTIN_NAME                30

char built_in[BUILTIN_OPERATIONS][MAX_BUILTIN_NAME] =
{
    "previous-line",
    "next-line",
    "scroll-down",
    "scroll-up",
    "beginning-of-list",
    "end-of-list",
    "next-signal",
    "SIGALRM",
    "SIGABRT",
    "SIGBUS",
    "SIGCHLD",
    "SIGCLD",
    "SIGCONT",
    "SIGEMT",
    "SIGFPE",
    "SIGHUP",
    "SIGILL",
    "SIGINFO",
    "SIGINT",
    "SIGIO",
    "SIGIOT",
    "SIGKILL",
    "SIGPIPE",
    "SIGPOLL",
    "SIGPROF",
    "SIGPWR",
    "SIGQUIT",
    "SIGSEGV",
    "SIGSTOP",
    "SIGSYS",
    "SIGTERM",
    "SIGTRAP",
    "SIGTSTP",
    "SIGTTIN",
    "SIGTTOU",
    "SIGURG",
    "SIGUSR1",
    "SIGUSR2",
    "SIGVTALRM",
    "SIGWINCH",
    "SIGXCPU",
    "SIGXFSZ",
    "kill-process",
    "refresh",
    "exit",
    "hard-refresh",
    "SIGSTKFLT",
    "horizontal-scroll-left",
    "horizontal-scroll-right",
};


void
remove_log()
{
    if (stdout_log_name)
	unlink(stdout_log_name);

    if (stderr_log_name)
	unlink(stderr_log_name);
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
    memcpy(global_buf + 2, header_text,
	   min(tty_columns - 2, (int)strlen(header_text)));

    tty_colors(HeaderBrightness, HeaderForeground, HeaderBackground);

    window_goto(header_window, 0, 0);
    window_puts(header_window, global_buf, tty_columns);
}


void
set_status(what)
   char *what;
{
    memset(global_buf, ' ', tty_columns);

    if (what)
	memcpy(global_buf, what, min(tty_columns, (int)strlen(what)));
    else
	memcpy(global_buf, help, min(tty_columns, (int)strlen(help)));

    tty_colors(StatusBrightness, StatusForeground, StatusBackground);

    window_goto(status_window, 0, 0);

    if (tty_columns < (int)((sizeof(sigdesc[0].signame) - 1) + 1))
	window_puts(status_window, global_buf, tty_columns);
    else
    {
	global_buf[tty_columns - 1 - (sizeof(sigdesc[0].signame)-1) - 1] = ' ';
	window_puts(status_window, global_buf,
		    tty_columns - (sizeof(sigdesc[0].signame) - 1) - 1);
    }
}


void
set_signal(index)
    int index;
{
    int i, len = sizeof(sigdesc[0].signame) - 1;

    if (index >= 0)
	for (i = 0; i < (int)(sizeof(sigdesc) / sizeof(xsignal_t)); i++)
	    if (sigdesc[i].signal == index)
	    {
		signal_type = i;
		break;
	    }

    if (tty_columns > len)
    {
	tty_colors(StatusBrightness, WHITE, StatusBackground);
	window_goto(status_window, 0, tty_columns - len - 1);
	window_puts(status_window, sigdesc[signal_type].signame, len);
	window_putc(status_window, ' ');
    }

    window_goto(status_window, 0, tty_columns - 1);
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

    set_status((char *)NULL);
    set_signal(-1);
    tty_update();
}


void
free_ps_list()
{
    int i;

    for (i = 0; i < processes; i++)
    {
	xfree(ps_vect[i]);
	ps_vect[i] = NULL;
    }
}


char *
read_ps_line(ps_output, line)
    FILE *ps_output;
    char *line;
{
    int c;
    char *ok;
    size_t lastchar;

    ok = fgets(line, MAX_LINE - 1, ps_output);

    if (ok == NULL)
	return NULL;

    if (line[lastchar = strlen(line) - 1] == '\n')
	line[lastchar] = 0;
    else
	while ((c = fgetc(ps_output)) != '\n' && c != EOF)
	    ;

    return ok;
}


int
get_PID_index(ps_output)
    FILE *ps_output;
{
    int i;
    char *h = header_text;

    if (read_ps_line(ps_output, header_text) == NULL)
	return -1;

    if (strstr(header_text, "PID") == NULL)
	return -1;

    for (i = 0; ; i++)
    {
	while (isspace((int)*h))
	    h++;

	if (memcmp(h, "PID", 3) == 0)
	    return i;

	while (!isspace((int)*h))
	    h++;
    }
}


int
kill_process(process_index)
    int process_index;
{
    int i;
    char *p;
    char pidstr[128];
    int pidnum;

    assert(process_index < processes);
    p = ps_vect[process_index];
    assert(p);

#ifdef __CYGWIN32__
    /* skip possible leading status char on cygwin ps */
    if((!isspace((int)*p)) && (!isdigit((int)*p)))
	p++;
#endif
    for (i=0; i < PID_index; i++)
    {
	while (isspace((int)*p))
	    p++;

	while (!isspace((int)*p))
	    p++;
    }

    i = 0;

    while (isspace((int)*p))
	p++;

    while (!isspace((int)*p))
	pidstr[i++] = *p++;

    pidstr[i] = 0;
    pidnum=atoi(pidstr);
    if(pidnum)
	return !kill(atoi(pidstr), sigdesc[signal_type].signal);
    else
	return -1;
}


void
build_ps_list(ps_output)
    FILE *ps_output;
{
    int i;
    char line[MAX_LINE];

    for (i = 0; read_ps_line(ps_output, line); i++)
    {
	ps_vect = (char **)xrealloc(ps_vect, (i + 1) * sizeof(char *));
	ps_vect[i] = xstrdup(line);
    }

    processes = i;
}


void
update_process(process, update_color)
    int process, update_color;
{
    assert(process < processes);

    int ps_length,visible_length,offset;
    ps_length=(int)strlen(ps_vect[process]);
    visible_length=(tty_columns-2);
    if(visible_length >= ps_length)
	offset=0;
    else
	offset=min(horizontal_offset, (ps_length-visible_length));
    memset(global_buf, ' ', tty_columns);
    memcpy(global_buf + 2, (ps_vect[process]+offset),
	   min(visible_length,strlen(ps_vect[process]+offset)));
    global_buf[0] = (process == current_process) ? '>' : ' ';
    global_buf[1] = ' ';

    if (update_color)
    {
	tty_brightness(ScreenBrightness);

	if (process == current_process)
	{
	    tty_foreground(ScreenBackground);
	    tty_background(ScreenForeground);
	}
	else
	{
	    tty_foreground(ScreenForeground);
	    tty_background(ScreenBackground);
	}
    }

    window_goto(processes_window, process - first_on_screen, 0);
    window_puts(processes_window, global_buf, tty_columns);
    window_goto(status_window, 0, tty_columns - 1);
}


void
update_all()
{
    int i;

    tty_colors(ScreenBrightness, ScreenForeground, ScreenBackground);

    if (tty_lines <= 4)
	return;

    window_goto(processes_window, 0, 0);

    for (i = first_on_screen;
	 i < processes && (i - first_on_screen < tty_lines - 3); i++)
	    if (i != current_process)
		update_process(i, OFF);
	    else
		window_goto(processes_window, i - first_on_screen, 0);

    update_process(current_process, ON);

    tty_colors(ScreenBrightness, ScreenForeground, ScreenBackground);

    memset(global_buf, ' ', tty_columns);

    for (; i - first_on_screen < tty_lines - 3; i++)
    {
	window_goto(processes_window, i - first_on_screen, 0);
	window_puts(processes_window, global_buf, tty_columns);
    }

    window_goto(status_window, 0, tty_columns - 1);
}


void
clean_up()
{
    tty_end_cursorapp();
    tty_end(NULL);
    remove_log();
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
ps(args)
    char **args;
{
    FILE *stdout_log=NULL, *stderr_log=NULL;
    int stdout_log_fd, stderr_log_fd;

    remove_log();
    /* See the comment in system.c on closing tty descriptors.  */
    int old_stdout = dup(1);
    int old_stderr = dup(2);

    close(1);
    close(2);

    strcpy(stdout_log_name,stdout_log_template);
    stdout_log_fd = mkstemp(stdout_log_name);
    if(stdout_log_fd != -1)
	stdout_log = fdopen(stdout_log_fd, "w");

    strcpy(stderr_log_name,stderr_log_template);
    stderr_log_fd = mkstemp(stderr_log_name);
    if(stderr_log_fd != -1)
	stderr_log = fdopen(stderr_log_fd, "w");

    if(!stdout_log || !stderr_log)
    {
	if(stdout_log)
	    fclose(stdout_log);
	if(stderr_log)
	    fclose(stderr_log);

	remove_log();

	dup(old_stdout);
	dup(old_stderr);

	close(old_stdout);
	close(old_stderr);
	fprintf(stderr, "%s: cannot write temp file: %s.\n",
		g_program, ((stdout_log==NULL) ? stdout_log_name : stderr_log_name));
	return 0;
    }

    if (ps_cmd)
	xfree(ps_cmd);

    if (args)
    {
	int i, bytes = 0;

	for (i = 0; args[i]; i++)
	    bytes += 1 + strlen(args[i]);

	ps_cmd = xmalloc(16 + bytes + 1);

	strcpy(ps_cmd, "ps");

	for (i = 0; args[i]; i++)
	{
	    strcat(ps_cmd, " ");
	    strcat(ps_cmd, args[i]);
	}
    }
    else
	ps_cmd = xstrdup("ps");

    if (system(ps_cmd) != 0)
    {
	fclose(stdout_log);
	fclose(stderr_log);

	remove_log();

	dup(old_stdout);
	dup(old_stderr);

	close(old_stdout);
	close(old_stderr);

	fprintf(stderr, "%s: invalid command line for ps(1).\n", g_program);
	fprintf(stderr, "%s: the command was: `%s'.\n", g_program, ps_cmd);
	fprintf(stderr, "%s: see the ps(1) man page for details.\n", g_program);

	return 0;
    }

    fclose(stdout_log);
    fclose(stderr_log);

    dup(old_stdout);
    dup(old_stderr);

    close(old_stdout);
    close(old_stderr);

    return 1;
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
		tty_key_list_insert((unsigned char *)key_seq, built_in[j]);
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
    int display_title = OFF;
    int display_header = OFF;
    int display_processes = OFF;
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
     *      1 line:	the status
     *      2 lines:	title + status
     *    3-4 lines:	title + header + status
     *	 >= 5 lines:	everything
     */

    global_buf = xrealloc(global_buf, tty_columns + 1);

    current_process = min(current_process, processes - 1);
    first_on_screen = max(0, current_process - (tty_lines - 3) + 1);
    assert(first_on_screen >= 0);

    window_resize(status_window, 0, tty_lines - 1, 1, tty_columns);

    if (tty_lines >= 2)
	display_title = ON;

    if (tty_lines >= 3)
	display_header = ON;

    if (tty_lines >= 5)
    {
	display_processes = ON;

	if (StartupScrollStep <= 0 || StartupScrollStep >= (tty_lines - 3) - 1)
	    scroll_step = (tty_lines - 3) / 2;
	else
	    scroll_step = StartupScrollStep;
    }

    window_resize(title_window, 0, 0, display_title ? 1 : 0, tty_columns);
    window_resize(header_window, 0, 1, display_header ? 1 : 0, tty_columns);
    window_resize(processes_window, 0, 2,
		  display_processes ? (tty_lines - 3) : 0, tty_columns);
}


/*
 * Resize (if necessary) and then refresh all gitps' components.
 */
void
refresh(signum)
    int signum;
{
    current_process = min(current_process, processes - 1);

    resize(0);

    if (signum == SIGCONT)
    {
	/* We were suspended.  Switch back to noncanonical mode.  */
	tty_set_mode(TTY_NONCANONIC);
	tty_defaults();
    }

    if (tty_lines == 4)
    {
	/* We know that when this is the case, the processes will not
	   be displayed and there will be an empty line that
	   potentially can contain some junk.  */
	tty_defaults();
	tty_clear();
    }

    set_title();
    set_header();
    set_status((char *)NULL);
    set_signal(-1);
    update_all();
    tty_update();

    if (signum == SIGCONT)
	tty_update_title(ps_cmd);
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
    printf("usage: %s [-hvilcbp]\n", g_program);
    printf(" -h         print this help message\n");
    printf(" -v         print the version number\n");
    printf(" -c         use ANSI colors\n");
    printf(" -b         don't use ANSI colors\n");
    printf(" -l         don't use the last screen character\n");
    printf(" -p         pass the remaining arguments to ps(1)\n");
}


int
main(argc, argv)
    int argc;
    char *argv[];
{
    char *tmp;
    int key, keys;
    tty_key_t *ks;
    FILE *stdout_log;
    int repeat_count;
    char **arguments;
    int i, no_of_arguments, exit_code = 0;
    int need_update, need_update_all, old_current_process;
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

	    case 'p':
		/* End the list of gitps arguments.  If this option is
		   given, the following arguments are to be passed to
		   `ps(1)'.  */
		goto done;

	    case '?':
		return 1;

	    default:
		fprintf(stderr, "%s: unknown error\n", g_program);
		return 1;
	}

  done:
    if (optind < argc)
    {
	no_of_arguments = argc - optind;
	arguments = (char **)xmalloc(sizeof(char *) * (no_of_arguments + 1));

	for (i = 0; i < no_of_arguments; i++)
	    arguments[i] = argv[optind++];

	arguments[i] = NULL;
    }
    else
	arguments = NULL;

    title_text = xmalloc(strlen(PRODUCT) + strlen(VERSION) + 64);
    sprintf(title_text, " %s %s - Process Viewer/Killer", PRODUCT, VERSION);

    tty_init(TTY_FULL_INPUT);

    common_configuration_init();
    use_section("[GITPS-Keys]");
    keys = read_keys(0);
    configuration_end();

    specific_configuration_init();
    use_section("[Setup]");

    temporary_directory = getenv("TMPDIR");

    if (temporary_directory == NULL)
	temporary_directory = "/tmp";

    if (ansi_colors == -1)
	AnsiColors = get_flag_var("AnsiColors", OFF);
    else
	AnsiColors = ansi_colors;

    if (use_last_screen_character)
	UseLastScreenChar = get_flag_var("UseLastScreenChar",  OFF);
    else
	UseLastScreenChar = OFF;

    tty_set_last_char_flag(UseLastScreenChar);
    StartupScrollStep  = get_int_var("StartupScrollStep", (tty_lines - 3) / 2);

    use_section("[GITPS-Setup]");
    help = get_string_var("Help", "");
    RefreshAfterKill = get_flag_var("RefreshAfterKill", ON);

    use_section(AnsiColors ? color_section : monochrome_section);
    get_colorset_var(PSColors, PSFields, PS_FIELDS);

    use_section("[GITPS-Keys]");
    keys = read_keys(keys);

    if (keys == MAX_KEYS)
	fprintf(stderr, "%s: too many key sequences; only %d are allowed.\n",
		g_program, MAX_KEYS);

    configuration_end();

#ifndef HAVE_LONG_FILE_NAMES
    fprintf(stderr,
	    "%s: warning: your system doesn't support long file names.",
	    g_program);
#endif /* !HAVE_LONG_FILE_NAMES */

    stdout_log_template = xmalloc(32 + strlen(temporary_directory) + 1);
    stderr_log_template = xmalloc(32 + strlen(temporary_directory) + 1);
    stdout_log_name     = xmalloc(32 + strlen(temporary_directory) + 1);
    stderr_log_name     = xmalloc(32 + strlen(temporary_directory) + 1);
    sprintf(stdout_log_template, "%s/gitps.1.XXXXXX", temporary_directory);
    sprintf(stderr_log_template, "%s/gitps.2.XXXXXX", temporary_directory);

    if (ps(arguments) == 0)
	return 1;

    tty_start_cursorapp();

    title_window  = window_init();
    header_window = window_init();
    processes_window = window_init();
    status_window = window_init();

    resize(0);

    tty_get_screen(screen);
    tty_set_mode(TTY_NONCANONIC);
    tty_defaults();

    signal_handlers(ON);

    first_on_screen = current_process = 0;

    tty_update_title(ps_cmd);
    set_signal(SIGTERM);

  restart:
    stdout_log = fopen(stdout_log_name, "r");
    remove_log();

    if ((PID_index = get_PID_index(stdout_log)) == -1)
    {
	exit_code = 1;
	goto end;
    }

    free_ps_list();
    build_ps_list(stdout_log);
    fclose(stdout_log);

    refresh(0);

    while (1)
    {
	while ((ks = tty_get_key(&repeat_count)) == NULL)
	    report_undefined_key();

	set_status((char *)NULL);
	set_signal(-1);

	key = ((char *)ks->aux_data - (char *)built_in) / MAX_BUILTIN_NAME;

	switch (key)
	{
	    case BUILTIN_previous_line:
		need_update_all = need_update = 0;

		while (repeat_count--)
		{
		    if (current_process != 0)
			current_process--;
		    else
			break;

		    if (current_process + 1 == first_on_screen)
		    {
			first_on_screen = max(0, first_on_screen -
					      scroll_step);
			need_update_all = 1;
		    }
		    else
		    {
			if (!need_update)
			    update_process(current_process + 1, ON);

			need_update = 1;
		    }
		}

		if (need_update_all)
		    update_all();
		else
		    if (need_update)
			update_process(current_process, ON);
		break;

	    case BUILTIN_next_line:
		need_update_all = need_update = 0;

		while (repeat_count--)
		{
		    if (current_process < processes - 1)
			current_process++;
		    else
			break;

		    if (current_process - first_on_screen >= tty_lines - 3)
		    {
			first_on_screen = min(first_on_screen +
					      scroll_step,
					      processes - 1 -
					      (tty_lines - 3) + 1);
			need_update_all = 1;
			continue;
		    }

		    if (!need_update)
			update_process(current_process - 1, ON);

		    need_update = 1;
		}

		if (need_update_all)
		    update_all();
		else
		    if (need_update)
			update_process(current_process, ON);
		break;

	    case BUILTIN_scroll_down:
		if (current_process == 0)
		    break;

		old_current_process = current_process;

		if (current_process < tty_lines - 3)
		    current_process = first_on_screen = 0;
		else
		{
		    current_process -= tty_lines - 3;
		    first_on_screen = max(0, first_on_screen - (tty_lines-3));
		}

		if (processes > tty_lines - 3)
		    update_all();
		else
		{
		    update_process(old_current_process, ON);
		    update_process(current_process, ON);
		}

		break;

	    case BUILTIN_scroll_up:
		if (current_process == processes - 1)
		    break;

		old_current_process = current_process;

		if (processes - 1 - first_on_screen < tty_lines - 3)
		    current_process = processes - 1;
		else
		    if (processes - 1 - current_process < tty_lines - 3)
		    {
			current_process = processes - 1;
			first_on_screen = processes - 1 - (tty_lines - 3) + 1;
		    }
		    else
		    {
			current_process += tty_lines - 3;
			first_on_screen = min(first_on_screen + tty_lines - 3,
					      (processes - 1) -
					      (tty_lines - 3) + 1);
		    }

		if (processes > tty_lines - 3)
		    update_all();
		else
		{
		    update_process(old_current_process, ON);
		    update_process(current_process, ON);
		}

		break;

	    case BUILTIN_horizontal_scroll_left:
		horizontal_offset--;
		if(horizontal_offset < 0)
		    horizontal_offset=0;
		update_all();
		break;

	    case BUILTIN_horizontal_scroll_right:
		horizontal_offset++;
		update_all();
		break;

	    case BUILTIN_beginning_of_list:
		if (current_process == 0)
		    break;

		current_process = first_on_screen = 0;
		update_all();
		break;

	    case BUILTIN_end_of_list:
		if (current_process == processes - 1)
		    break;

		current_process = processes - 1;
		first_on_screen = max(0, (processes-1) - (tty_lines-3) + 1);
		update_all();
		break;

	    case BUILTIN_next_signal:
		signal_type++;
		signal_type %= sizeof(sigdesc) / sizeof(xsignal_t);
		set_signal(-1);
		break;

	    case BUILTIN_SIGALRM:   set_signal(SIGALRM);   break;
	    case BUILTIN_SIGABRT:   set_signal(SIGABRT);   break;
#ifdef SIGBUS
	    case BUILTIN_SIGBUS:    set_signal(SIGBUS);    break;
#endif
#ifdef SIGCHLD
	    case BUILTIN_SIGCHLD:   set_signal(SIGCHLD);   break;
#endif
#ifdef SIGCLD
	    case BUILTIN_SIGCLD:    set_signal(SIGCLD);    break;
#endif
#ifdef SIGCONT
	    case BUILTIN_SIGCONT:   set_signal(SIGCONT);   break;
#endif
#ifdef SIGEMT
	    case BUILTIN_SIGEMT:    set_signal(SIGEMT);    break;
#endif
#ifdef SIGFPE
	    case BUILTIN_SIGFPE:    set_signal(SIGFPE);    break;
#endif
	    case BUILTIN_SIGHUP:    set_signal(SIGHUP);    break;
#ifdef SIGILL
	    case BUILTIN_SIGILL:    set_signal(SIGILL);    break;
#endif
#ifdef SIGINFO
	    case BUILTIN_SIGINFO:   set_signal(SIGINFO);   break;
#endif
	    case BUILTIN_SIGINT:    set_signal(SIGINT);    break;
#ifdef SIGIO
	    case BUILTIN_SIGIO:     set_signal(SIGIO);     break;
#endif
#ifdef SIGIOT
	    case BUILTIN_SIGIOT:    set_signal(SIGIOT);    break;
#endif
	    case BUILTIN_SIGKILL:   set_signal(SIGKILL);   break;
	    case BUILTIN_SIGPIPE:   set_signal(SIGPIPE);   break;
#ifdef SIGPOLL
	    case BUILTIN_SIGPOLL:   set_signal(SIGPOLL);   break;
#endif
#ifdef SIGPROF
	    case BUILTIN_SIGPROF:   set_signal(SIGPROF);   break;
#endif
#ifdef SIGPWR
	    case BUILTIN_SIGPWR:    set_signal(SIGPWR);    break;
#endif
	    case BUILTIN_SIGQUIT:   set_signal(SIGQUIT);   break;
	    case BUILTIN_SIGSEGV:   set_signal(SIGSEGV);   break;
#ifdef SIGSTOP
	    case BUILTIN_SIGSTOP:   set_signal(SIGSTOP);   break;
#endif
#ifdef SIGSYS
	    case BUILTIN_SIGSYS:    set_signal(SIGSYS);    break;
#endif
	    case BUILTIN_SIGTERM:   set_signal(SIGTERM);   break;
#ifdef SIGTRAP
	    case BUILTIN_SIGTRAP:   set_signal(SIGTRAP);   break;
#endif
#ifdef SIGTSTP
	    case BUILTIN_SIGTSTP:   set_signal(SIGTSTP);   break;
#endif
#ifdef SIGTTIN
	    case BUILTIN_SIGTTIN:   set_signal(SIGTTIN);   break;
#endif
#ifdef SIGTTOU
	    case BUILTIN_SIGTTOU:   set_signal(SIGTTOU);   break;
#endif
#ifdef SIGURG
	    case BUILTIN_SIGURG:    set_signal(SIGURG);    break;
#endif
	    case BUILTIN_SIGUSR1:   set_signal(SIGUSR1);   break;
	    case BUILTIN_SIGUSR2:   set_signal(SIGUSR2);   break;
#ifdef SIGVTALRM
	    case BUILTIN_SIGVTALRM: set_signal(SIGVTALRM); break;
#endif
#ifdef SIGWINCH
	    case BUILTIN_SIGWINCH:  set_signal(SIGWINCH);  break;
#endif
#ifdef SIGXCPU
	    case BUILTIN_SIGXCPU:   set_signal(SIGXCPU);   break;
#endif
#ifdef SIGXFSZ
	    case BUILTIN_SIGXFSZ:   set_signal(SIGXFSZ);   break;
#endif
#ifdef SIGSTKFLT
	    case BUILTIN_SIGSTKFLT: set_signal(SIGSTKFLT); break;
#endif
	    case BUILTIN_hard_refresh:
		tty_touch();

	    case BUILTIN_refresh:
		ps(arguments);
		goto restart;

	    case BUILTIN_exit:
		goto end;

	    case BUILTIN_kill_process:
		if (kill_process(current_process))
		{
		    if(RefreshAfterKill)
		    {
			struct timespec tv;
			tv.tv_sec=0;
			tv.tv_nsec=5 * 1000 * 1000 ; /* half a second */
			nanosleep(&tv,NULL);
			ps(arguments);
			goto restart;
		    }
		}
		else
		{
		    int e = errno;

		    tty_beep();
		    memset(global_buf, ' ', tty_columns);
		    tmp = xmalloc(16 + strlen((e == EPERM) ? no_perm:no_proc));
		    sprintf(tmp, "Error: %s", (e == EPERM) ? no_perm:no_proc);
		    set_status(tmp);
		    tty_update();
		    xfree(tmp);
		    errno = 0;
		    tty_get_key(NULL);
		    set_status((char *)NULL);
		    set_signal(-1);
		}
		break;

	    default:
		report_undefined_key();
		break;
	}
    }

  end:
    remove_log();
    tty_set_mode(TTY_CANONIC);
    tty_end(screen);
    return exit_code;
}
