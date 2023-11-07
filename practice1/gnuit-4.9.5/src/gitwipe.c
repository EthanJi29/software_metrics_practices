/* gitwipe.c -- An utility that deletes the contents of a file in order to
   make it impossible for someone to recover it.  */

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "xtime.h"

#include <sys/types.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include "file.h"
#include <fcntl.h>
#include <limits.h>


#define WIPE_BUFFER_SIZE        64*1024
#undef min
#define min(a, b) ((a) <= (b) ? (a) : (b))


char *g_program;


void
usage()
{
    fprintf(stderr, "%s file1 [file2 [...]]\n", g_program);
    exit(1);
}


off64_t
file_length(fd)
    int fd;
{
    off64_t current, length;

    current = lseek64(fd, 0, SEEK_CUR);
    length  = lseek64(fd, 0, SEEK_END);
    lseek64(fd, current, SEEK_SET);
    return length;
}


int
wipe(file)
    char *file;
{
    int fd;
    unsigned char *buf;
    off64_t len, i;
    ssize_t j, bytes_to_write;

    fd = open64(file, O_RDWR | O_BINARY);

    if (fd == -1)
    {
	fprintf(stderr, "%s: can't open file '%s'.\n", g_program, file);
	return 1;
    }

    len = file_length(fd);

    if (len == 0)
	return 0;

    buf = (unsigned char *)malloc(WIPE_BUFFER_SIZE);

    if (buf == NULL)
    {
	fprintf(stderr, "%s: virtual memory exhausted.\n", g_program);
	return 1;
    }

    for (i = 0; i < len; i += WIPE_BUFFER_SIZE)
    {
	bytes_to_write = min(len - i, WIPE_BUFFER_SIZE);

	for (j = 0; j < bytes_to_write; j++)
	    buf[j] = rand() % 0xFF;

	if (write(fd, buf, bytes_to_write) != bytes_to_write)
	{
	    fprintf(stderr, "%s: can't write to file %s.\n", g_program, file);
	    return 1;
	}
    }

    close(fd);
    sync();

    /* Don't delete the file! The file system might notice that the blocks
       in this file are no longer used and never write them back to disk.
       And there is more: sync() may return before the actual writing is
       done.  See the Linux sync(2) & sync(8) manual pages for more detail.
       If you want to be sure, REBOOT NOW ! :-))))))))))))))  */

    return 0;
}


/*
 * argv[1...]   = files
 * return value = no of errors (unprocessed files)
 */

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int i, errors = 0;

    g_program = argv[0];

    if (argc < 2)
	usage();

    srand(time(NULL));

    for (i = 1; i < argc; i++)
	errors += wipe(argv[i]);

    return (errors != 0);
}
