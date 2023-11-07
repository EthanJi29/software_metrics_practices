/* signal.c -- Signals management for git*.  */

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <assert.h>

#include "signals.h"
#include "tty.h"
#include "misc.h"


/* Set on SIGINT.  Should be reset when detected.  */
int user_heart_attack;

/* Set if there are pending suspend/window change signals.  */
static int suspend_requested;
static int refresh_requested;
static int alarm_requested;

static int refresh_at_SIGCONT;
static int signals_allowed = OFF;
static int job_control = ON;

static void install_handler PROTO ((int));


extern void hide PROTO (());
extern void refresh PROTO ((int));
extern void clock_refresh PROTO ((int));


/*
 * Service pending signals (signals that can modify git at specific
 * times - in our case SIGTSTP (suspend) and SIGWINCH (window change)).
 */
void
service_pending_signals()
{
#ifdef SIGSTOP
    if (suspend_requested)
    {
	hide();
	kill(getpid(), SIGSTOP);
	suspend_requested = 0;
	refresh_requested = 0;
	alarm_requested = 0;
	return;
    }
#endif /* SIGSTOP */

#ifdef SIGWINCH
    if (refresh_requested)
    {
	tty_defaults();
	tty_io_clear();
	refresh(SIGWINCH);
	refresh_requested = 0;
	alarm_requested = 0;
	return;
    }
#endif /* SIGWINCH */

    if (alarm_requested)
    {
	/* FIXME: do something.  */
	/* clock_refresh(SIGALRM); ??? */
	alarm_requested = 0;
	return;
    }

    /* Add here as needed...  */
}


/*
 * Activate/deactivate suspend/resize signals.
 */
void
signals(mode)
    int mode;
{
    signals_allowed = mode;

    if (signals_allowed)
	service_pending_signals();
}


#ifdef SIGSTOP

static RETSIGTYPE
suspend(signum)
    int signum;
{
    if (signals_allowed)
    {
	refresh_at_SIGCONT = (tty_get_mode() == TTY_NONCANONIC);
	hide();
	kill(getpid(), SIGSTOP);
	suspend_requested = 0;
    }
    else
	suspend_requested = 1;

    /* Do this last, to avoid trouble on old Unix systems.  On newer
       systems it doesn't matter, and on older ones I rather quit
       because hide() is not reentrant.  */
    install_handler(signum);
}

#endif /* SIGSTOP */


#ifdef SIGWINCH

static RETSIGTYPE
window_change(signum)
    int signum;
{
    if (signals_allowed)
    {
	tty_defaults();
	tty_io_clear();
	refresh(signum);
	refresh_requested = 0;
    }
    else
	refresh_requested = 1;

    /* Do this last, to avoid trouble on old Unix systems.  On newer
       systems it doesn't matter, and on older ones I rather quit
       because refresh() is not reentrant.  */
    install_handler(signum);
}

#endif /* SIGWINCH */


static RETSIGTYPE
resume(signum)
    int signum;
{
    if (refresh_at_SIGCONT)
    {
	refresh(signum);
	refresh_requested = 0;
    }

    /* Do this last, to avoid trouble on old Unix systems.  On newer
       systems it doesn't matter, and on older ones I would rather
       quit because refresh() is not reentrant.  */
    install_handler(signum);
}


static RETSIGTYPE
time_change(signum)
    int signum;
{
    if (signals_allowed)
    {
	if (get_local_time()->tm_sec == 0)
	    clock_refresh(signum);
	tty_key_print_async();
	alarm_requested = 0;
    }
    else
	alarm_requested = 1;

    /* Do this last, to avoid trouble on old Unix systems.  On newer
       systems it doesn't matter, and on older ones I rather quit
       because clock_refresh() is not reentrant.  */
    install_handler(signum);

    /* Schedule the next alarm.  */
    alarm(60 - get_local_time()->tm_sec);
}


static RETSIGTYPE
panic(signum)
    int signum;
{
    signal(signum, panic);
    user_heart_attack = 1;
}


static void
install_handler(signum)
    int signum;
{
#ifdef HAVE_POSIX_SIGNALS
    sigset_t mask;
    struct sigaction action;

    sigemptyset(&mask);

    /* Block other terminal-generated signals while handler runs.  */
    sigaddset(&mask, SIGINT);
    action.sa_mask = mask;
    action.sa_flags = 0;

    switch (signum)
    {
#if defined(SIGTSTP) && defined (SIGCONT)
	case SIGTSTP:
#ifdef SIGWINCH
	    sigaddset(&mask, SIGWINCH);
#endif /* SIGWINCH */
	    sigaddset(&mask, SIGCONT);
	    sigaddset(&mask, SIGALRM);
	    action.sa_handler = suspend;
	    break;

	case SIGCONT:
#ifdef SIGWINCH
	    sigaddset(&mask, SIGWINCH);
#endif /* SIGWINCH */
	    sigaddset(&mask, SIGTSTP);
	    sigaddset(&mask, SIGALRM);
	    action.sa_handler = resume;
	    break;
#endif /* SIGTSTP && SIGCONT */

#ifdef SIGWINCH
	case SIGWINCH:
#if defined(SIGTSTP) && defined (SIGCONT)
	    sigaddset(&mask, SIGTSTP);
	    sigaddset(&mask, SIGCONT);
#endif /* SIGTSTP && SIGCONT */
	    sigaddset(&mask, SIGALRM);
	    action.sa_handler = window_change;
	    break;
#endif /* SIGWINCH */

	case SIGALRM:
#if defined(SIGTSTP) && defined (SIGCONT)
	    sigaddset(&mask, SIGTSTP);
	    sigaddset(&mask, SIGCONT);
#endif /* SIGTSTP && SIGCONT */
#ifdef SIGWINCH
	    sigaddset(&mask, SIGWINCH);
#endif /* SIGWINCH */
	    action.sa_handler = time_change;
	    break;

	default:
	    assert(0);
    }

    sigaction(signum, &action, NULL);
#else /* ! HAVE_POSIX_SIGNALS */
    /* No POSIX signals.  That's bad, we have no way of blocking
       signals during the execution of the signal handlers.
       Theoretically, it is possible for a signal to be delivered
       during the execution of another handler, potentially leading to
       disastrous results.  */
    switch (signum)
    {
#if defined(SIGTSTP) && defined (SIGCONT)
	case SIGTSTP:
	    signal(SIGTSTP, suspend);
	    break;

	case SIGCONT:
	    signal(SIGCONT, resume);
	    break;
#endif /* SIGTSTP && SIGCONT */

#ifdef SIGWINCH
	case SIGWINCH:
	    signal(SIGWINCH, window_change);
	    break;
#endif /* SIGWINCH */

	case SIGALRM:
	    signal(SIGALRM, time_change);
	    break;

	default:
	    assert(0);
    }
#endif /* HAVE_POSIX_SIGNALS */
}


void
signal_handlers(status)
    int status;
{
    if (status == ON)
    {
#if defined(SIGTSTP) && defined(SIGCONT)
	/* Job control stuff. */
	if (job_control)
	{
	    install_handler(SIGTSTP);
	    install_handler(SIGCONT);
	}
#endif /* SIGTSTP && SIGCONT */

#ifdef SIGWINCH
	/* Handle window changes.  */
	install_handler(SIGWINCH);
#endif /* SIGWINCH */

	install_handler(SIGALRM);
    }
    else
    {
#if defined(SIGTSTP) && defined(SIGCONT)
	/* Job control stuff. */
	if (job_control)
	{
	    signal(SIGTSTP, SIG_IGN);
	    signal(SIGCONT, SIG_IGN);
	}
#endif /* SIGTSTP && SIGCONT */

#ifdef SIGWINCH
	/* Ignore window changes.  */
	signal(SIGWINCH, SIG_IGN);
#endif /* SIGWINCH */

	signal(SIGALRM, SIG_IGN);
    }
}


/*
 * Call this at the very beginning to avoid receiving signals before
 * being able to handle them.
 */
void
signals_init()
{
#if defined(SIGTSTP) && defined(SIGCONT)
    /* Job control stuff. */
    job_control = (signal(SIGTSTP, SIG_IGN) != SIG_IGN);
    signal(SIGCONT, SIG_IGN);
#endif /* SIGTSTP && SIGCONT */

#ifdef SIGWINCH
    /* Ignore window changes for now.  */
    signal(SIGWINCH, SIG_IGN);
#endif /* SIGWINCH */

    /* Miscelaneous signals that we want to handle.  */
    signal(SIGSEGV, fatal_signal);
    signal(SIGHUP,  fatal_signal);
    signal(SIGTERM, fatal_signal);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGINT,  panic);
}
