/* configure.c -- Configuration files management functions.  */

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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else /* !HAVE_STDLIB_H */
#include "ansi_stdlib.h"
#endif /* !HAVE_STDLIB_H */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include "file.h"
#include <ctype.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "xstring.h"
#include "tty.h"
#include "configure.h"
#include "misc.h"


static FILE *fileptr;
static int   sectionptr;
static char  line[MAXLINE];


int
configuration_init(file_name)
    char *file_name;
{
    if (file_name == NULL)
	return 0;

    fileptr = fopen(file_name, "r");

    if (fileptr == NULL)
	return 0;

    sectionptr = -1;
    return 1;
}


void
configuration_end()
{
    if (fileptr)
	fclose(fileptr);
}


static int
configuration_getline()
{
    int c;
    size_t len;
    char *comment;

    /* loop until we get a line that isn't a comment */
    for(;;)
    {
	comment=NULL;
	if (fgets(line, MAXLINE, fileptr) == NULL)
	    return 0;

	if ((len = strlen(line)) == MAXLINE - 1)
	{
	    fprintf(stderr, "%s: configuration: line too long. Truncated.\n",
		    g_program);

	    /* Search the end of this big line.  */
	    for (;;)
	    {
		c = fgetc(fileptr);
		if (c == '\n' || c == EOF)
		    break;
	    }
	}

	if ((comment = strchr(line, ICS)))
	{
	    char *ptr;
	    int isws=1;
	    *comment = 0;
	    /* handle lines with whitespace before comment */
	    for(ptr=line;*ptr;ptr++)
	    {
		if(!isspace((int)*ptr))
		{
		    isws=0;
		    break;
		}
	    }
	    if(isws)
		comment=line; /* make whitespace part of comment */
	}
	else
	    if (line[len - 1] == '\n')
		line[len - 1] = 0;
	if(comment != line)
	    return 1;
    }
}


int
configuration_section(section_name)
    char *section_name;
{
    fseek(fileptr, 0, SEEK_SET);

    while (configuration_getline())
	if (strcmp(section_name, line) == 0)
	    return sectionptr = ftell(fileptr);

    return sectionptr = -1;
}


void
configuration_getvarinfo(var_name, dest, fields, seek)
    char *var_name, **dest;
    int fields, seek;
{
    int fld;
    char buf[MAXLINE], *ptr, *tmp;

    if (seek)
	fseek(fileptr, sectionptr, SEEK_SET);

    if (fields == 1)
	*dest = 0;
    else
	memset((char *)dest, 0, fields * sizeof(char *));

    while (configuration_getline() && *line)
    {
	*buf = 0;
	sscanf(line, "%s", buf);

	if (seek == NO_SEEK)
	    buf[32] = 0;        /* Just in case... */

	if (!isprint((int)*buf))
	    return;

	if (seek == NO_SEEK || strcmp(var_name, buf) == 0)
	{
	    if ((ptr = strchr(line, IAS)) && *++ptr)
	    {
		for (dest[0] = ptr, fld = 1; *ptr && fld < fields; ptr++)
		    if (*ptr == IFS)
		    {
			*ptr = 0;
			if (*(ptr + 1) && *(ptr + 1) != IFS)
			    dest[fld] = ptr + 1;
			fld++;
		    }
		if ((ptr = strchr(ptr, IFS)))
		    *ptr = 0;
	    }

	    for (fld = 0; fld < fields; fld++)
		if (dest[fld])
		{
		    while (isspace((int)*dest[fld]))
			dest[fld]++;

		    tmp = dest[fld] + strlen(dest[fld]) - 1;

		    while (isspace((int)*tmp) && tmp >= dest[fld])
			tmp--;

		    *(tmp + 1) = 0;

		    if (dest[fld][0] == 0)
			dest[fld] = NULL;
		}

	    if (seek == NO_SEEK)
		strcpy(var_name, buf);
	    return;
	}
    }

    if (seek == NO_SEEK)
	*var_name = 0;
}
