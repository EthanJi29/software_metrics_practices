/* status.c -- A simple status line management file.  */

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

#ifdef HAVE_UTSNAME
#include <sys/utsname.h>
#endif

#include <assert.h>

#include "xtime.h"

#include "xstring.h"
#include "xmalloc.h"
#include "window.h"
#include "status.h"
#include "configure.h"
#include "tty.h"
#include "misc.h"


extern int AnsiColors;


static window_t *status_window;
static char *status_message;
static char status_type;
static char status_alignment;
static char *status_buffer;
static char *status_default_message;

#ifdef HAVE_UTSNAME
static struct utsname u;
#endif


#define STATUSBAR_FIELDS        9

static char *StatusBarFields[STATUSBAR_FIELDS] =
{
    "StatusBarForeground",
    "StatusBarBackground",
    "StatusBarBrightness",
    "StatusBarWarningForeground",
    "StatusBarWarningBackground",
    "StatusBarWarningBrightness",
    "StatusBarErrorForeground",
    "StatusBarErrorBackground",
    "StatusBarErrorBrightness"
};

#ifdef HAVE_LINUX
static int StatusBarColors[STATUSBAR_FIELDS] =
{
    BLACK, CYAN, OFF, BLACK, WHITE, OFF, WHITE, RED, ON
};
#else   /* !HAVE_LINUX */
static int StatusBarColors[STATUSBAR_FIELDS] =
{
    BLACK, WHITE, OFF, BLACK, WHITE, OFF, BLACK, WHITE, ON
};
#endif  /* !HAVE_LINUX */

#define StatusBarForeground             StatusBarColors[0]
#define StatusBarBackground             StatusBarColors[1]
#define StatusBarBrightness             StatusBarColors[2]
#define StatusBarWarningForeground      StatusBarColors[3]
#define StatusBarWarningBackground      StatusBarColors[4]
#define StatusBarWarningBrightness      StatusBarColors[5]
#define StatusBarErrorForeground        StatusBarColors[6]
#define StatusBarErrorBackground        StatusBarColors[7]
#define StatusBarErrorBrightness        StatusBarColors[8]


void
status_init(default_message)
    char *default_message;
{
    use_section(AnsiColors ? color_section : monochrome_section);

    get_colorset_var(StatusBarColors, StatusBarFields, STATUSBAR_FIELDS);

    status_default_message = xstrdup(default_message);
    toprintable(status_default_message, strlen(status_default_message));
    status_window = window_init();

#ifdef HAVE_UTSNAME
    uname(&u);
#endif
}


void
status_end()
{
    window_end(status_window);
}


void
status_resize(columns, line)
    size_t columns, line;
{
    if (status_buffer)
	xfree(status_buffer);

    status_buffer = xmalloc(columns * sizeof(char));

    window_resize(status_window, 0, line, 1, columns);
}


static void
build_message()
{
    int i, j;
    struct tm *time;
    char date_str[32];
    char *ptr, *temp_msg;
    size_t len, temp_msg_len;

    assert(status_message);

    memset(status_buffer, ' ', status_window->columns);
    temp_msg = xmalloc(temp_msg_len = (strlen(status_message) + 1));

    for (i = 0, j = 0; status_message[i]; i++)
	if (status_message[i] == '\\')
	    switch (status_message[i + 1])
	    {
		case 'h' :
#ifdef HAVE_UTSNAME
		    ptr = u.nodename;
#else
		    ptr = "";
#endif
		    goto get_system_info;

		case 's' :
#ifdef HAVE_UTSNAME
		    ptr = u.sysname;
#else
		    ptr = "";
#endif
		    goto get_system_info;

		case 'm' :
#ifdef HAVE_UTSNAME
		    ptr = u.machine;
#else
		    ptr = "";
#endif
		  get_system_info:
		    if (ptr[0])
		    {
			len = strlen(ptr);
			temp_msg = xrealloc(temp_msg, temp_msg_len += len);
			memcpy(&temp_msg[j], ptr, len);
		    }
		    else
		    {
			len = 6;
			temp_msg = xrealloc(temp_msg, temp_msg_len += len);
			memcpy(&temp_msg[j], "(none)", len);
		    }

		    j += len;
		    i++;
		    break;

		case 'd' :
		    time = get_local_time();
		    sprintf(date_str, "%s %s %02d %d",
			    day_name[time->tm_wday], month_name[time->tm_mon],
			    time->tm_mday, time->tm_year + 1900);
		    len = strlen(date_str);
		    temp_msg = xrealloc(temp_msg, temp_msg_len += len);
		    memcpy(&temp_msg[j], date_str, len);
		    j += len;
		    i++;
		    break;

		case '\\':
		    temp_msg[j++] = '\\';
		    i++;
		    break;

		case '\0':
		    temp_msg[j++] = '\\';
		    break;

		default:
		    temp_msg[j++] = '\\';
		    temp_msg[j++] = status_message[++i];
		    break;
	    }
	else
	{
	    if (status_message[i] == '\t')
	    {
		temp_msg = xrealloc(temp_msg, temp_msg_len += 8);
		memcpy(&temp_msg[j], "        ", 8);
		j += 8;
	    }
	    else
		temp_msg[j++] = status_message[i];
	}

    temp_msg[j] = 0;

    len = strlen(temp_msg);

    if (status_alignment == STATUS_CENTERED &&
	(int)len < status_window->columns)
	memcpy(status_buffer + ((status_window->columns - len) >> 1),
	       temp_msg, len);
    else
	memcpy(status_buffer, temp_msg, min((int)len, status_window->columns));

    xfree(temp_msg);

    for (i = 0; i < status_window->columns; i++)
	if (status_buffer[i] == '\r' || status_buffer[i] == '\n')
	    status_buffer[i] = ' ';
}


void
status_update()
{
    tty_status_t status;
    tty_save(&status);

    build_message();

    switch (status_type)
    {
	case STATUS_WARNING:
	    tty_colors(StatusBarWarningBrightness,
		       StatusBarWarningForeground,
		       StatusBarWarningBackground);
	    break;

	case STATUS_ERROR:
	    tty_colors(StatusBarErrorBrightness,
		       StatusBarErrorForeground,
		       StatusBarErrorBackground);
	    break;

	default:
	    tty_colors(StatusBarBrightness,
		       StatusBarForeground,
		       StatusBarBackground);
	    break;
    }

    window_goto(status_window, 0, 0);
    window_puts(status_window, status_buffer, status_window->columns);

    tty_restore(&status);
}


void
status(message, type, alignment)
    char *message;
    int type, alignment;
{
    if (status_message)
	xfree(status_message);

    status_message = xstrdup(message);
    toprintable(status_message, strlen(status_message));
    status_type = type;
    status_alignment = alignment;

    status_update();
}


void
status_default()
{
    status(xstrdup(status_default_message), STATUS_OK, STATUS_CENTERED);
}
