/* xstack.c -- Simple stacks management file.  */

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

#include "xmalloc.h"
#include "xstring.h"
#include "xstack.h"


/*
 * Create a new stack.
 */

xstack_t *
xstack_init(esize)
    int esize;
{
    xstack_t *s = (xstack_t *)xcalloc(1, sizeof(xstack_t));

    s->esize = esize;
    return s;
}


/*
 * Destroy a stack.
 */

void
xstack_end(stack)
    xstack_t *stack;
{
    if (stack == NULL)
	return;

    if (stack->data)
	xfree(stack->data);

    xfree(stack);
}


/*
 * Push one element to a stack.
 */

void
xstack_push(stack, data)
    xstack_t *stack;
    void *data;
{
    stack->data = (void *)xrealloc(stack->data, ++stack->point * stack->esize);

    memcpy(((char *)stack->data) + (stack->point - 1) * stack->esize,
	   data,
	   stack->esize);
}


/*
 * Pop one element from a stack.
 */

void *
xstack_pop(stack, data)
    xstack_t *stack;
    void *data;
{
    if (stack->point == 0)
	return NULL;

    memcpy(data,
	   ((char *)stack->data) + --stack->point * stack->esize,
	   stack->esize);

    stack->data = (void *)xrealloc(stack->data, stack->point * stack->esize);

    return data;
}


/*
 * Preview the stack element located at 'offset' elements from the stack
 * point.  The stack point is located at offset = 0.
 */

void *
xstack_preview(stack, data, offset)
    xstack_t *stack;
    void *data;
    int offset;
{
    if (stack->point == 0 || offset > stack->point)
	return NULL;

    memcpy(data,
	   ((char *)stack->data) + (stack->point - offset) * stack->esize,
	   stack->esize);

    return data;
}


/*
 * Truncate a stack to 'point' elements.
 */

void
xstack_truncate(stack, point)
    xstack_t *stack;
    int point;
{
    stack->point = point;
    stack->data = (void *)xrealloc(stack->data, stack->point * stack->esize);
}


/*
 * Return the stack point.
 */

int
xstack_point(stack)
    xstack_t *stack;
{
    return stack->point;
}
