/* xstack.h -- Prototypes and #defines for the stuff in xstack.c.  */

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

#ifndef _GIT_XSTACK_H
#define _GIT_XSTACK_H


#include "stdc.h"


typedef struct
{
    void *data;         /* the stack data.  */
    int esize;          /* # of bytes in a stack element.  */
    int point;          /* # of elements in the stack.  */
} xstack_t;


extern xstack_t *xstack_init PROTO ((int));
extern void xstack_end PROTO ((xstack_t *));

extern void  xstack_push PROTO ((xstack_t *, void *));
extern void *xstack_pop PROTO ((xstack_t *, void *));
extern void *xstack_preview PROTO ((xstack_t *, void *, int));
extern void  xstack_truncate PROTO ((xstack_t *, int));
extern int xstack_point PROTO ((xstack_t *));


#endif  /* _GIT_XSTACK_H */
