/* gitkeys.c -- An utility designed to help users to find out what is the
   escape sequences sent by a particular key.  Users can use this to set up
   their configuration files.  */

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
#include <sys/ioctl.h>


#ifdef HAVE_POSIX_TTY
#include <termios.h>
#else
#ifdef HAVE_SYSTEMV_TTY
#include <termio.h>
#else
#include <sgtty.h>
#endif
#endif

#include <signal.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "stdc.h"
#include "xio.h"


/* Stolen from GNU Emacs.  */
#ifdef _POSIX_VDISABLE
#define CDISABLE _POSIX_VDISABLE
#else /* not _POSIX_VDISABLE */
#ifdef CDEL
#undef CDISABLE
#define CDISABLE CDEL
#else /* not CDEL */
#define CDISABLE 255
#endif /* not CDEL */
#endif /* not _POSIX_VDISABLE */


#ifdef HAVE_LINUX
speed_t ospeed;
#else   /* !HAVE_LINUX */
short ospeed;
#endif  /* !HAVE_LINUX */


#define TTY_OUTPUT 1


#ifdef HAVE_POSIX_TTY
static struct termios old_term;
static struct termios new_term;
#else
#ifdef HAVE_SYSTEMV_TTY
static struct termio old_term;
static struct termio new_term;
#else
static struct sgttyb  old_arg;
static struct tchars  old_targ;
static struct ltchars old_ltarg;
static struct sgttyb  new_arg;
static struct tchars  new_targ;
static struct ltchars new_ltarg;

/* NextStep doesn't define TILDE.  */
#ifndef TILDE
#define TILDE 0
#endif

#endif /* HAVE_SYSTEMV_TTY */
#endif /* HAVE_POSIX_TTY */


char *copyright =
"%s %s - Display key sequence utility\n\
GIT is free software; you can redistribute it and/or modify it under the\n\
terms of the GNU General Public License as published by the Free Software\n\
Foundation; either version 2, or (at your option) any later version.\n\
Copyright (C) 1993-1998 Free Software Foundation, Inc.\n\
Written by Tudor Hulubei and Andrei Pitis, students at PUB, Romania\n\
\nPress SPACE when done.\n\n";


extern RETSIGTYPE do_exit PROTO ((int));


void
tty_init()
{
    /* This simply doesn't fit into the current scheme.  Maybe the tt
       stuff should be moved into a separate library.  This is clearly
       a self contained function.  */
    {
#ifdef HAVE_POSIX_TTY
	tcgetattr(TTY_OUTPUT, &old_term);

	new_term = old_term;
	new_term.c_iflag &= ~(IXON | ICRNL | IGNCR | INLCR | IGNBRK | BRKINT);
	new_term.c_oflag &= ~OPOST;
	new_term.c_lflag |= ISIG | NOFLSH;
	new_term.c_lflag &= ~(ICANON | ECHO);

	/* I think we will always have these ones:  */

	new_term.c_cc[VINTR] = CDISABLE;
	new_term.c_cc[VQUIT] = CDISABLE;

#ifdef VSTART
	new_term.c_cc[VSTART] = CDISABLE;
#endif

#ifdef VSTOP
	new_term.c_cc[VSTOP]  = CDISABLE;
#endif

	new_term.c_cc[VMIN]  = 1;
	new_term.c_cc[VTIME] = 0;

	/* ... but not always these ones: (in fact I am not sure if I
	   really need to overwrite all these, but just in case... */

#ifdef VERASE
	new_term.c_cc[VERASE] = CDISABLE;
#endif

#ifdef VKILL
	new_term.c_cc[VKILL] = CDISABLE;
#endif

#ifdef VEOL
	new_term.c_cc[VEOL] = CDISABLE;
#endif

#ifdef VEOL2
	new_term.c_cc[VEOL2] = CDISABLE;
#endif

#ifdef VSWTCH
	new_term.c_cc[VSWTCH] = CDISABLE;
#endif

#ifdef VSUSP
	new_term.c_cc[VSUSP] = CDISABLE;
#endif

#ifdef VDSUSP
	new_term.c_cc[VDSUSP] = CDISABLE;
#endif

#ifdef VREPRINT
	new_term.c_cc[VREPRINT] = CDISABLE;
#endif

#ifdef VDISCARD
	new_term.c_cc[VDISCARD] = CDISABLE;
#endif

#ifdef VWERASE
	new_term.c_cc[VWERASE] = CDISABLE;
#endif

#ifdef VLNEXT
	new_term.c_cc[VLNEXT] = CDISABLE;
#endif

	tcsetattr(TTY_OUTPUT, TCSADRAIN, &new_term);
	ospeed = cfgetospeed(&new_term);
#else
#ifdef HAVE_SYSTEMV_TTY
	ioctl(TTY_OUTPUT, TCGETA, &old_term);

	new_term = old_term;
	new_term.c_iflag &= ~(IXON | ICRNL | IGNCR | INLCR);
	new_term.c_oflag = 0;
	new_term.c_lflag = 0;

	/* I think we will always have these:  */

	new_term.c_cc[VINTR] = CDISABLE;
	new_term.c_cc[VQUIT] = CDISABLE;

#ifdef VSTART
	new_term.c_cc[VSTART] = CDISABLE;
#endif

#ifdef VSTOP
	new_term.c_cc[VSTOP] = CDISABLE;
#endif

	new_term.c_cc[VMIN] = 1;
	new_term.c_cc[VTIME] = 0;

	/* ... but not always these:  (in fact I am not sure if I really
	   need to overwrite all these, but just in case... */

#ifdef VERASE
	new_term.c_cc[VERASE] = CDISABLE;
#endif

#ifdef VKILL
	new_term.c_cc[VKILL] = CDISABLE;
#endif

#ifdef VEOL
	new_term.c_cc[VEOL] = CDISABLE;
#endif

#ifdef VEOL2
	new_term.c_cc[VEOL2] = CDISABLE;
#endif

#ifdef VSWTCH
	new_term.c_cc[VSWTCH] = CDISABLE;
#endif

#ifdef VSUSP
	new_term.c_cc[VSUSP] = CDISABLE;
#endif

#ifdef VDSUSP
	new_term.c_cc[VDSUSP] = CDISABLE;
#endif

#ifdef VREPRINT
	new_term.c_cc[VREPRINT] = CDISABLE;
#endif

#ifdef VDISCARD
	new_term.c_cc[VDISCARD] = CDISABLE;
#endif

#ifdef VWERASE
	new_term.c_cc[VWERASE] = CDISABLE;
#endif

#ifdef VLNEXT
	new_term.c_cc[VLNEXT] = CDISABLE;
#endif

	ioctl(TTY_OUTPUT, TCSETAW, &new_term);
	ospeed = new_term.c_cflag & CBAUD;
#else
	ioctl(TTY_OUTPUT, TIOCGETP, &old_arg);
	ioctl(TTY_OUTPUT, TIOCGETC, &old_targ);
	ioctl(TTY_OUTPUT, TIOCGLTC, &old_ltarg);

	new_arg   = old_arg;
	new_targ  = old_targ;
	new_ltarg = old_ltarg;
	new_arg.sg_flags = ((old_arg.sg_flags &
			 ~(ECHO | CRMOD | XTABS | ALLDELAY | TILDE)) | CBREAK);
	new_targ.t_intrc   = CDISABLE;
	new_targ.t_quitc   = CDISABLE;
	new_targ.t_stopc   = CDISABLE;
	new_targ.t_startc  = CDISABLE;
	new_targ.t_eofc    = CDISABLE;
	new_targ.t_brkc    = CDISABLE;
	new_ltarg.t_lnextc = CDISABLE;
	new_ltarg.t_flushc = CDISABLE;
	new_ltarg.t_werasc = CDISABLE;
	new_ltarg.t_rprntc = CDISABLE;
	new_ltarg.t_dsuspc = CDISABLE;	  /* DSUSPC (delayed SUSPC, ^Y).  */
	new_ltarg.t_suspc  = CDISABLE;

	ioctl(TTY_OUTPUT, TIOCSETN, &new_arg);
	ioctl(TTY_OUTPUT, TIOCSETC, &new_targ);
	ioctl(TTY_OUTPUT, TIOCSLTC, &new_ltarg);
	ospeed = new_arg.sg_ospeed;
#endif /* HAVE_SYSTEMV_TTY */
#endif /* HAVE_POSIX_TTY */

/* Try to make sure the terminal is not locked.  */
#ifdef TCXONC
#ifdef __QNX__
	{
	    int value = 1;
	    ioctl (TTY_OUTPUT, TCXONC, &value);
	}
#else
	ioctl(TTY_OUTPUT, TCXONC, 1);
#endif
#endif

#ifndef APOLLO
#ifdef TIOCSTART
	ioctl(TTY_OUTPUT, TIOCSTART, 0);
#endif
#endif

#ifdef HAVE_POSIX_TTY
#ifdef TCOON
	tcflow(TTY_OUTPUT, TCOON);
#endif
#endif
    }
}


void
tty_end()
{
#ifdef HAVE_POSIX_TTY
    tcsetattr(TTY_OUTPUT, TCSADRAIN, &old_term);
#else
#ifdef HAVE_SYSTEMV_TTY
    ioctl(TTY_OUTPUT, TCSETAW, &old_term);
#else
    ioctl(TTY_OUTPUT, TIOCSETN, &old_arg);
    ioctl(TTY_OUTPUT, TIOCSETC, &old_targ);
    ioctl(TTY_OUTPUT, TIOCSLTC, &old_ltarg);
#endif /* HAVE_SYSTEMV_TTY */
#endif /* HAVE_POSIX_TTY */
}


RETSIGTYPE
do_exit(signum)
    int signum;
{
    signal(signum, do_exit);
    tty_end();
    exit(1);
}


int
main()
{
    char c;

    printf(copyright, PRODUCT, VERSION);
    signal(SIGTERM, do_exit);

    tty_init();

#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);
#endif

#ifdef SIGCONT
    signal(SIGCONT, SIG_IGN);
#endif

    for (;;)
    {
	read(0, &c, 1);

	if (c == ' ')
	    break;

	printf("%x ", (unsigned char)c);
	fflush(stdout);
    }

    tty_end();
    printf("\n");
    return 0;
}
