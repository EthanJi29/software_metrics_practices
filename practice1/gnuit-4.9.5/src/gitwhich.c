/* gitwhich.c -- A simplified version of `which'.  */

/* Copyright (C) 1999, 2006-2007 Free Software Foundation, Inc.

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

/* Written by Tudor Hulubei, based on the `which' code by Paul Vixie.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else /* !HAVE_STDLIB_H */
#include "ansi_stdlib.h"
#endif /* !HAVE_STDLIB_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "xmalloc.h"
#include "xstring.h"

/* for gnulib lib/error.c */
char *program_name;

void
fatal(postmsg)
    char *postmsg;
{
    postmsg = NULL;
    exit(1);
}


/*
 * Return 1 if the executable exists, 0 otherwise.
 */
int
find(name, path)
    char *name;
    char *path;
{
    int found = 0;
    char *pc = path;

    while (*pc != '\0' && found == 0)
    {
	char save;
	int len = 0;
	char *tmp;

	while (*pc != ':' && *pc != '\0')
	{
	    len++;
	    pc++;
	}

	save = *pc;
	*pc = '\0';
	tmp = xmalloc(strlen(pc - len) + 1 + strlen(name) + 1);
	sprintf(tmp, "%s/%s", pc - len, name);
	*pc = save;
	if (*pc)
	    pc++;

	found = (access(tmp, 1) == 0);
	xfree(tmp);
    }

    return found;
}


/*
 * Some systems don't have `which'...
 */

int
main(argc, argv)
    int argc;
    char *argv[];
{
    program_name=argv[0];
    char *path = getenv("PATH");

    for (argc--, argv++;  argc;  argc--, argv++)
	if (find(*argv, path))
	    return 0;

    return 1;
}
