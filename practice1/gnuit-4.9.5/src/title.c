/* title.c -- title bar.  */

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

#if HAVE_UTIME_H
#include <utime.h>
#else /* !HAVE_UTIME_H */
#include <sys/utime.h>
#endif /* !HAVE_UTIME_H */

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include <assert.h>
#include <ctype.h>

#include "window.h"
#include "xmalloc.h"
#include "xstring.h"
#include "xio.h"
#include "tty.h"
#include "title.h"
#include "misc.h"


char *TitleFields[TITLE_FIELDS] =
{
    "TitleForeground",
    "TitleBackground",
    "TitleBrightness",
    "UserName",
    "TtyName",
    "ClockForeground",
    "ClockBackground",
    "ClockBrightness",
};

#ifdef HAVE_LINUX
int TitleColors[TITLE_FIELDS] =
{
    CYAN, BLUE, ON, YELLOW, YELLOW
};
#else   /* !HAVE_LINUX */
int TitleColors[TITLE_FIELDS] =
{
    WHITE, BLACK, ON, WHITE, WHITE
};
#endif  /* !HAVE_LINUX */

#define TitleForeground TitleColors[0]
#define TitleBackground TitleColors[1]
#define TitleBrightness TitleColors[2]
#define UserName        TitleColors[3]
#define TtyName         TitleColors[4]
#define ClockForeground TitleColors[5]
#define ClockBackground TitleColors[6]
#define ClockBrightness TitleColors[7]


static char *product_name;

static int product_name_length;
static int login_name_length;
static int tty_device_length;

/* The length of the "User: tudor  tty: /dev/ttyp0" info string.
   It also includes 6 characters for the clock.  */
static int info_length;

static char login_string[] = "User:";
static char ttydev_string[] = "tty:";

static char mail_have_none[] = "";
static char mail_have_mail[] = "(Mail)";
static char mail_have_new[] = "(New Mail)";

static char *mail_string = "";
static char *mail_file=NULL;
static off64_t mail_size=0;
static time_t mail_mtime=0;

static window_t *title_window;

extern int in_terminal_mode PROTO (());

static int calc_info_length PROTO (());
static int mail_check PROTO (());


static int
calc_info_length()
{
    info_length = (sizeof(login_string)  - 1) + 1 + login_name_length + 1 +
	          (strlen(mail_string)) + 1 +
		  (sizeof(ttydev_string) - 1) + 1 + tty_device_length + 1 +
		  6 + 1;
    return(info_length);
}


void
title_init()
{
    product_name = xmalloc(1 + strlen(PRODUCT) + 1 + strlen(VERSION) + 1);
    sprintf(product_name, " %s %s", PRODUCT, VERSION);
    product_name_length = strlen(product_name);
    login_name_length = strlen(login_name);
    tty_device_length = strlen(tty_device);

    mail_file=getenv("MAIL");
    if(mail_file)
    {
	struct stat s;
	if(xstat(mail_file,&s) != -1)
	{
	    mail_size=s.st_size;
	    mail_mtime=s.st_mtime;
	}
    }
    mail_check();
    info_length = calc_info_length();
    title_window = window_init();
}


void
title_end()
{
    window_end(title_window);
}


void
title_resize(columns, line)
    size_t columns, line;
{
    window_resize(title_window, 0, line, 1, columns);
}


/*
 * Update the title clock and check for mail.  If signum is 0 it means that
 * clock_refresh() has been called synchronously and no terminal
 * flushing is necessary at this point.
 */

void
clock_refresh(signum)
    int signum;
{
    int hour;
    char buf[16];
    struct tm *time;
    int line, column;
    tty_status_t status;

    if (in_terminal_mode())
	return;

    if (product_name_length + 2 + info_length >= title_window->columns)
	return;

    /* signum means we weren't called from title_update */
    if(signum && mail_check())
    {
	title_update();
    }
	    

    time = get_local_time();

    tty_save(&status);
    tty_get_cursor(&line, &column);

    tty_cursor(OFF);

    if ((hour = time->tm_hour % 12) == 0)
	hour = 12;

    sprintf(buf, "%2d:%02d%c", hour, time->tm_min,
	    (time->tm_hour < 12) ? 'a' : 'p');
    window_goto(title_window, 0, title_window->columns - 7);
    tty_colors(ClockBrightness, ClockForeground, ClockBackground);
    window_puts(title_window, buf, strlen(buf));

    tty_goto(line, column);
    tty_restore(&status);

    if (signum)
	tty_update();
}

static int
mail_check()
{
    char *old_mail=mail_string;
    mail_string=mail_have_none;
    int total=0;
    int read=0;
    int inheaders=0;
    int gotstatus=0;
    FILE *mbox;
#define TMPBUFSIZE 2048
    char line[TMPBUFSIZE];
    struct stat s;
    struct utimbuf utbuf;

    if(!mail_file)
	return 0;
    if(xstat(mail_file,&s) == -1)
	return 0;
    utbuf.actime=s.st_atime;
    utbuf.modtime=s.st_mtime;

    mbox=fopen(mail_file,"r");
    if(!mbox)
	return 0;

    while(fgets(line,TMPBUFSIZE,mbox))
    {
	if(strcmp(line,"")==0)
	    inheaders=0;
	else if(strncmp(line,"From ",strlen("From ")) == 0)
	{
	    inheaders=1;
	    gotstatus=0;
	    total++;
	}
	else if(inheaders && !gotstatus &&
		(strncmp(line,"Status:",strlen("Status:"))==0))
	{
	    char *status=strchr(line,':');
	    status++;
	    while(*status && isspace(*status))
		status++;
	    if(*status)
	    {
		gotstatus=1;
		if(strchr(status,'R'))
		    read++;
	    }
	}
    }
    fclose(mbox);
    utime(mail_file,&utbuf);

    if(total)
    {
	if(total-read)
	{
	    mail_string=mail_have_new;
	}
	else
	{
	    mail_string=mail_have_mail;
	}
    }
    info_length=calc_info_length();
    if(strcmp(mail_string,old_mail) == 0)
	return 0; /* No change  */
    else
	return 1; /* need to update title */
}

void
title_update()
{
    int length;
    char *buf;
    tty_status_t status;

    tty_save(&status);

    tty_colors(TitleBrightness, TitleForeground, TitleBackground);

    window_goto(title_window, 0, 0);
    window_puts(title_window, product_name, product_name_length);

    buf = xmalloc(title_window->columns + 1);

    mail_check();
    if (product_name_length + 2 + info_length < title_window->columns)
    {
	length = title_window->columns - product_name_length - info_length;

	assert(length > 0);

	memset(buf, ' ', length);
	window_puts(title_window, buf, length);

	window_goto(title_window, 0, product_name_length + length);
	window_puts(title_window, login_string, sizeof(login_string) - 1);
	window_putc(title_window, ' ');

	tty_foreground(UserName);
	window_puts(title_window, login_name, login_name_length);
	window_putc(title_window, ' ');

	window_puts(title_window, mail_string, strlen(mail_string));

	window_putc(title_window, ' ');

	tty_foreground(TitleForeground);
	window_puts(title_window, ttydev_string, sizeof(ttydev_string) - 1);
	window_putc(title_window, ' ');

	tty_foreground(TtyName);
	window_puts(title_window, tty_device, tty_device_length);

	tty_foreground(TitleForeground);
	window_putc(title_window, ' ');

	clock_refresh(0);

	window_goto(title_window, 0, title_window->columns - 1);
	window_putc(title_window, ' ');
    }
    else if (product_name_length < title_window->columns)
    {
	length = title_window->columns - product_name_length;
	memset(buf, ' ', length);
	window_puts(title_window, buf, length);
    }

    xfree(buf);

    tty_restore(&status);
}
