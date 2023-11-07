/* xmalloc.c -- Safe memory management routines.  Includes xmalloc, xcalloc,
   xrealloc and free.  fatal() is called when there is no more memory
   available.  */

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

#include <sys/types.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include "xmalloc.h"


extern void fatal PROTO ((char *));

void 
xalloc_die(void)
{
    fatal("virtual memory exhausted");
    /* NOTREACHED */
    abort();
}

void
xfree(pointer)
    void *pointer;
{
    if (pointer)
	free(pointer);
    else
	fatal("xfree: trying to free NULL");
}
