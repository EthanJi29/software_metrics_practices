/* misc.c -- Miscelaneous functions used in git/gitps/gitview.  */

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

#include <signal.h>
#include <ctype.h>
#include <sys/types.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include <pwd.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "xstring.h"
#include "xmalloc.h"
#include "configure.h"
#include "file.h"
#include "tty.h"
#include "misc.h"


static char   SYSTEM_CONFIGFILE_PREFIX[] = "/gnuitrc.";
static char     USER_CONFIGFILE_PREFIX[] = "/.gnuitrc.";
static char OLD_USER_CONFIGFILE_PREFIX[] = "/.gitrc.";

static char *termdir;
static char *bindir;

char *login_name;


char *day_name[] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};


char *month_name[] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


/* The pointer to the head of the file type attributes list.  */
file_type_info_t *fti_head = NULL;

extern void fatal PROTO ((char *));


void
compute_directories()
{
    char *prefix_relative_bin_dir = "/bin";
    char *prefix_relative_term_dir = "/share/gnuit";

    bindir = xmalloc(strlen(PREFIX) + strlen(prefix_relative_bin_dir) + 1);
    sprintf(bindir, "%s%s", PREFIX, prefix_relative_bin_dir);
    termdir = xmalloc(strlen(PREFIX) + strlen(prefix_relative_term_dir) + 1);
    sprintf(termdir, "%s%s", PREFIX, prefix_relative_term_dir);
}


/*
 * Add $(bindir) to the PATH environment variable so that git will be
 * able to find its auxiliary programs/scripts even when that
 * directory is not in the PATH.  I don't think it is worth searching
 * $(bindir) in the PATH first...
 */

void
update_path()
{
    char *path = getenv("PATH");

    if (path == NULL)
	xsetenv("PATH", bindir);
    else
    {
	char *new_value = xmalloc(strlen(path) + 1 + strlen(bindir) + 1);

	sprintf(new_value, "%s:%s", path, bindir);
	xsetenv("PATH", new_value);
	xfree(new_value);
    }
}


void
display_exit_message(signame)
    char *signame;
{
    struct tm *time = get_local_time();

    fprintf(stderr, "%s %d %2d:%02d:%02d %s[%d]: exiting on %s signal\n",
	    month_name[time->tm_mon], time->tm_mday, time->tm_hour,
	    time->tm_min, time->tm_sec, g_program, (int)getpid(), signame);
}


extern void clean_up PROTO (());


RETSIGTYPE
fatal_signal(signum)
    int signum;
{
    clean_up();

    switch (signum)
    {
	case SIGTERM:
	    display_exit_message("TERM");
	    break;

	case SIGHUP:
	case SIGINT:
	    display_exit_message((signum == SIGHUP) ? "HUP" : "INT");
	    break;

	case SIGSEGV:
	    display_exit_message("SEGV");
	    goto ask_report;

	default:
	    fprintf(stderr,
		    "%s: got a stupid signal (%d). Unless it was a joke ...\n",
		    g_program, signum);
	  ask_report:
	    fprintf(stderr, "%s: please report to gnuit-dev@gnu.org\n",
		    g_program);
	    break;
    }

    exit(signum);
}


void
configuration_fatal_error(configfile)
    char *configfile;
{
    fprintf(stderr, "%s: installation problem: \n", g_program);
    fprintf(stderr, "%s: cannot find configuration file '%s'.\n\n",
	    g_program, configfile);
}


void
configuration_warning(configfile)
    char *configfile;
{
    fprintf(stderr,
	    "\n%s: Cannot open configuration file '%s'.\n",
	    g_program, configfile);
    fprintf(stderr,
	    "%s: See the info documentation for details.\n",
	    g_program);
    fprintf(stderr,
	   "%s: If the TERM environment variable is, say, vt102, your\n",
	    g_program);
    fprintf(stderr,
	    "%s: configuration file name is 'gnuitrc.vt102'.\n",
	    g_program);
    fprintf(stderr,
	   "%s: You can copy a configuration file in your home directory\n",
	    g_program);
    fprintf(stderr,
	    "%s: and modify it in order to overwrite the default one.\n",
	    g_program);
    fprintf(stderr,
	    "%s: Add a dot at the start of the file, e.g. '~/.gnuitrc.xterm'.\n",
	    g_program);
    fprintf(stderr,
	    "%s: Try modifying 'gnuitrc.generic'...\n\n",
	    g_program);
}


void
common_configuration_init()
{
    /* Load the global gnuitrc.common file.  */
    char *configfile = xmalloc(strlen(termdir) + 1 +
			       strlen(SYSTEM_CONFIGFILE_PREFIX) +
			       sizeof("common") + 1);
    strcpy(configfile, termdir);
    strcat(configfile, SYSTEM_CONFIGFILE_PREFIX);
    strcat(configfile, "common");

    if (configuration_init(configfile) == 0)
    {
	/* Give up if global gnuitrc.common is not found.  */
	configuration_fatal_error(configfile);
	exit(1);
    }
}


int
specific_configuration_init()
{
    char *configfile = xmalloc(strlen(g_home) + 1 + strlen(USER_CONFIGFILE_PREFIX) +
			       strlen(tty_type) + 1);
    strcpy(configfile, g_home);
    strcat(configfile, USER_CONFIGFILE_PREFIX);
    strcat(configfile, tty_type);

    if (configuration_init(configfile) == 0)
    {
	xfree(configfile);
	configfile = xmalloc(strlen(g_home) + 1 +
			     strlen(OLD_USER_CONFIGFILE_PREFIX) +
			     strlen(tty_type) + 1);
	strcpy(configfile, g_home);
	strcat(configfile, OLD_USER_CONFIGFILE_PREFIX);
	strcat(configfile, tty_type);

	if (configuration_init(configfile) == 0)
	{
	    xfree(configfile);
	    configfile = xmalloc(strlen(termdir) + 1 +
				 strlen(SYSTEM_CONFIGFILE_PREFIX) +
				 strlen(tty_type) + 1);
	    strcpy(configfile, termdir);
	    strcat(configfile, SYSTEM_CONFIGFILE_PREFIX);
	    strcat(configfile, tty_type);

	    if (configuration_init(configfile) == 0)
	    {
		configuration_warning(configfile);

		xfree(configfile);
		configfile = xmalloc(strlen(termdir) + 1 +
				     strlen(SYSTEM_CONFIGFILE_PREFIX) +
				     sizeof("generic") + 1);
		strcpy(configfile, termdir);
		strcat(configfile, SYSTEM_CONFIGFILE_PREFIX);
		strcat(configfile, "generic");

		if (configuration_init(configfile) == 0)
		{
		    configuration_fatal_error(configfile);
		    exit(1);
		}
		return 0;
	    }
	}
    }

    xfree(configfile);

    return 1;
}


void
use_section(section)
    char *section;
{
     char *gitfmprefix="[GITFM-";
     char *gitprefix="[GIT-";
     if (configuration_section(section) == -1)
     {
	  /* If we are looking for [GITFM-something,
	     accept [GIT-something for backwards compatibility */
	  if(strncmp(section,gitfmprefix,strlen(gitfmprefix)) == 0)
	  {
	       char *newsection=xmalloc(strlen(gitprefix) +
					strlen(section+strlen(gitprefix)) + 1);
	       strcpy(newsection,gitprefix);
	       strcat(newsection,section+strlen(gitfmprefix));
	       if (configuration_section(newsection) != -1)
	       {
		    /* found old [GIT-something section */
		    xfree(newsection);
		    return;
	       }
	       xfree(newsection);
	  }

	  fprintf(stderr,
		  "%s: can't find section %s in the configuration file.\n",
		  g_program, section);
	  exit(1);
    }
}


int
get_int_var(var_name, default_value)
    char *var_name;
    int default_value;
{
    char *data;

    configuration_getvarinfo(var_name, &data, 1, DO_SEEK);

    return data ? atoi(data) : default_value;
}


int
get_const_var(var_name, options, options_no, default_value)
    char *var_name, *options[];
    int options_no, default_value;
{
    int i;
    char *data;

    configuration_getvarinfo(var_name, &data, 1, DO_SEEK);

    if (data)
    {
	for (i = 0; i < options_no; i++)
	    if (strcmp(data, options[i]) == 0)
		break;

	if (i == options_no)
	    fprintf(stderr, "%s: invalid %s (%s).\n", g_program, var_name, data);
	else
	    return i;
    }

    return default_value;
}


int
get_flag_var(var_name, default_value)
    char *var_name;
    int default_value;
{
    char *data;

    configuration_getvarinfo(var_name, &data, 1, DO_SEEK);

    if (data)
    {
	if (strcmp(data, "ON")  == 0)
	    return 1;

	if (strcmp(data, "OFF") == 0)
	    return 0;

	fprintf(stderr, "%s: invalid %s (%s).\n", g_program, var_name, data);
	return default_value;
    }

    return default_value;
}


char *
get_string_var(var_name, default_value)
    char *var_name, *default_value;
{
    char *data;

    configuration_getvarinfo(var_name, &data, 1, DO_SEEK);

    if (data)
	return xstrdup(data);

    return default_value;
}


void
get_colorset_var(charset, colorset_name, fields_no)
    int *charset;
    char *colorset_name[];
    int fields_no;
{
    int i, index;
    char *data;

    for (i = 0; i < fields_no; i++)
    {
	configuration_getvarinfo(colorset_name[i], &data, 1, DO_SEEK);

	if (data)
	{
	    index = tty_get_color_index(data);
	    if (index == -1)
		fprintf(stderr, "%s: invalid %s (%s).\n",
			g_program, colorset_name[i], data);
	    else
		charset[i] = index;
	}
    }
}


/*
 * Minimize the path, that is, convert paths like /etc/./././///./ into
 * their cleaner form: /etc.  Stolen from the Thix kernel.  Enhanced to
 * minimize paths like /usr/local/bin/../lib as well.
 */

char *
minimize_path(path)
    char *path;
{
    char *cpath = path;
    char *opath = path;

    if (*opath == '/')
	*cpath++ = *opath++;
    else
	fatal("relative path encountered");

    while (*opath)
    {
	while (*opath == '/' ||
	       (*opath == '.' &&
		(*(opath + 1) == '/' || *(opath + 1) == '\0')))
	    opath++;

	if (*opath == '.' && *(opath + 1) == '.' &&
	    (*(opath + 2) == '/' || *(opath + 2) == '\0'))
	{
	    /* This is something like /usr/local/bin/.. and we are
	       going to remove the trailing .. along with the
	       directory that no longer meakes sense: bin.  */

	    /* Check for /..  and do nothing if this is the case.  */
	    if (cpath - 1 != path)
	    {
		for (cpath -= 2; *cpath != '/'; cpath--);
		cpath++;
	    }

	    opath += 2;
	    continue;
	}

	while (*opath && *opath != '/')
	    *cpath++ = *opath++;

	if (*opath)
	    *cpath++ = '/';
    }

    if (*(cpath - 1) == '/' && cpath - path > 1)
	cpath--;

    *cpath = '\0';
    return path;
}


void
get_login_name()
{
    struct passwd *pwd;
    int euid = geteuid();

    if ((pwd = getpwuid(euid)) == NULL)
    {
	fprintf(stderr,
		"%s: OOOPS, I can't get your user name (euid = %d)!\n",
		g_program, euid);
	fprintf(stderr,
		"%s: Your account no longer exists or you are a %s",
		g_program, "SYSTEM CRACKER! :-)\n");
	fprintf(stderr, "%s: Correct the problem and try again.\n", g_program);
	exit(1);
    }

    login_name     = xstrdup(pwd->pw_name);
}


void
truncate_long_name(name, dest, len)
    char *name, *dest;
    int len;
{
    int name_len;

    switch (len)
    {
	case 0:
	    break;

	case 1:
	    dest[0] = ' ';
	    break;

	case 2:
	    dest[0] = dest[1] = ' ';
	    break;

	case 3:
	    dest[0] = dest[1] = dest[2] = ' ';
	    break;

	default:
	    name_len = strlen(name);

	    if (name_len > len)
	    {
		dest[0] = dest[1] = dest[2] = '.';
		memcpy(dest + 3, name + name_len - len + 3, len - 3);
	    }
	    else
		memcpy(dest, name, name_len);
	    break;
    }
}


char *
truncate_string(path, temppath, len)
    char *path;
    char *temppath;
    int len;
{
    truncate_long_name(path, temppath, len - 1);
    temppath[min(len - 1, (int)strlen(path))] = '\0';
    return temppath;
}


off64_t
get_file_length(fd)
    int fd;
{
    off64_t current, length;

    current = lseek64(fd, 0, SEEK_CUR);
    length  = lseek64(fd, 0, SEEK_END);
    lseek64(fd, current, SEEK_SET);
    return length;
}


struct tm *
get_local_time()
{
    time_t __time;

    /* Get the broken-down time representation.  */
    __time = time(NULL);
    return localtime(&__time);
}


/*
 * Set the environment variable `variable' to the given `value'.
 * Remove the previous value, if any.
 * Return 0 on success, -1 on failure.
 */

int
xsetenv(variable, value)
    char *variable;
    char *value;
{
    int result;

#ifdef HAVE_PUTENV
    {
	char *environment_string =
	    xmalloc(strlen(variable) + 1 + strlen(value) + 1);
	sprintf(environment_string, "%s=%s", variable, value);
	result = putenv(environment_string);
    }
#else
#ifdef HAVE_SETENV
    unsetenv(variable);
    result = setenv(variable, value, 1);
#else
	#error
#endif /* HAVE_SETENV */
#endif /* HAVE_PUTENV */

    if (result == -1)
	fprintf(stderr, "%s: warning: cannot add '%s' to environment\n",
		g_program, variable);

    return result;
}


/*
 * Make sure there are only printable characters in `string'.
 */

void
toprintable(string, length)
    char *string;
    size_t length;
{
    size_t i;

    for (i = 0; i < length; i++)
	if (!isprint((int)string[i]))
	    string[i] = '?';
}


int
needs_quotes(string, length)
    char *string;
    size_t length;
{
    size_t i;

    for (i = 0; i < length; i++)
	if (!isalnum((int)string[i]) &&
	    string[i] != '.' && string[i] != '-' &&
	    string[i] != '+' && string[i] != '=' &&
	    string[i] != '~' && string[i] != '^' &&
	    string[i] != '%' && string[i] != '@' &&
	    string[i] != '/' && string[i] != ':' &&
	    string[i] != '{' && string[i] != '}' &&
	    string[i] != ',' && string[i] != '_')
	    return 1;

    return 0;
}


/*
 * Check if it is a background command.  We are doing this by looking
 * for a '&' at the end of the command string.
 */

int
is_a_bg_command(cmd)
    char *cmd;
{
    int i;

    for (i = strlen(cmd) - 1; i >= 0; i--)
    {
	if (cmd[i] == '&')
	    return 1;

	/* Skip spaces and tabs.  */
	if (cmd[i] != ' ' && cmd[i] != key_TAB)
	    return 0;
    }

    /* No '&' found.  */
    return 0;
}


/*
 * Check if it is a empty command (containing only spaces and ';'s.
 * My guess that almost any command that doesn't contain at least one
 * alphanumeric character should be considered as being empty, but I
 * don't want to take any chances.
 */

int
is_an_empty_command(cmd)
    char *cmd;
{
    for (; *cmd; cmd++)
	if (*cmd != ' ' && *cmd != ';')
	    return 0;

    return 1;
}


void
get_file_type_info()
{
    char *contents[3];
    char pattern[80];
    int brightness, foreground, background;
    file_type_info_t *previous = NULL, *fti, *fti_head1 = NULL;


    for (;;)
    {
	configuration_getvarinfo(pattern, contents, 3, NO_SEEK);

	if (*pattern == '\0')
	    break;

	if (contents[0])
	    foreground = tty_get_color_index(contents[0]);
	else
	    foreground = -1;

	if (contents[1])
	    background = tty_get_color_index(contents[1]);
	else
	    background = -1;

	if (contents[2])
	    brightness = tty_get_color_index(contents[2]);
	else
	    brightness = -1;

	/* Insert the file type entry just obtained into the list.  */
	fti = (file_type_info_t *)xmalloc(sizeof(file_type_info_t));

	if (fti_head1 == NULL)
	    fti_head1 = previous = fti;
	else
	    previous->next = fti;

	fti->pattern    = xstrdup(pattern);
	fti->foreground = foreground;
	fti->background = background;
	fti->brightness = brightness;
	fti->next       = NULL;

	previous = fti;
    }

    /* Fixed by Marian Ciobanu <ciobi@liis.sfos.ro>, October 24, 1995.  */
    if (fti_head1)
    {
	if (fti_head)
	{
	    previous->next = fti_head;
	    fti_head = fti_head1;
	}
	else
	    fti_head = fti_head1;
    }
}
