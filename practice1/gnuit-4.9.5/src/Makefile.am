EXTRA_DIST = gitaction gitmkdirs gitmount gitrgrep gitunpack gitxgrep\
	     .gitaction .gdbinit git.transition gitdpkgname

bin_PROGRAMS = gitfm gitps gitview gitwipe gitkeys gitwhich
bin_SCRIPTS = gitaction gitmkdirs gitmount gitrgrep gitunpack gitxgrep\
	      .gitaction gitdpkgname

# We're not including tilde.c & tilde.h here because they are
# automagically detected (configure.in includes them in LIBOBJS).
EXTRA_gitfm_SOURCES = ansi_stdlib.h file.h stat.h stdc.h xtime.h

gitfm_SOURCES = git.c tty.c tty.h window.c window.h inputline.c inputline.h\
	      status.c status.h panel.c panel.h title.c title.h\
	      configure.c configure.h misc.c misc.h signals.c signals.h\
	      xmalloc.c xmalloc.h xstring.h xio.c xio.h\
	      xstack.c xstack.h system.c system.h \
	      tilde.c tilde.h history.c history.h
gitps_SOURCES = gitps.c tty.c tty.h window.c window.h misc.c misc.h\
		configure.c configure.h signals.c signals.h xio.c xio.h\
		xmalloc.c xmalloc.h xstring.h
gitview_SOURCES = gitview.c tty.c tty.h window.c window.h misc.c misc.h\
		  configure.c configure.h signals.c signals.h xio.c xio.h\
		  xmalloc.c xmalloc.h xstring.h
gitwipe_SOURCES = gitwipe.c
gitkeys_SOURCES = gitkeys.c
gitwhich_SOURCES = gitwhich.c xmalloc.c xmalloc.h

gitfm_LDADD = -L../lib -lgnu $(LIB_CLOCK_GETTIME) $(LIB_NANOSLEEP) $(LIBINTL)
gitps_LDADD = -L../lib -lgnu $(LIB_CLOCK_GETTIME) $(LIB_NANOSLEEP) $(LIBINTL)
gitview_LDADD = -L../lib -lgnu $(LIBINTL)
gitwhich_LDADD = -L../lib -lgnu $(LIBINTL)

AM_CPPFLAGS = -I$(srcdir)/.. -I.. -I$(top_srcdir)/lib -I$(top_srcdir)/src

if DEBIAN
AM_CPPFLAGS += -DDEBIAN
gitfm_LDFLAGS    = -Wl,-z,defs -Wl,--as-needed
gitps_LDFLAGS    = -Wl,-z,defs -Wl,--as-needed
gitview_LDFLAGS  = -Wl,-z,defs -Wl,--as-needed
gitwipe_LDFLAGS  = -Wl,-z,defs -Wl,--as-needed
gitkeys_LDFLAGS  = -Wl,-z,defs -Wl,--as-needed
gitwhich_LDFLAGS = -Wl,-z,defs -Wl,--as-needed
endif

install-exec-hook:
if TRANSITION
	$(INSTALL_SCRIPT) $(srcdir)/git.transition $(DESTDIR)$(bindir)/git
endif
	ln -sf gitrgrep $(DESTDIR)$(bindir)/gitregrep
	ln -sf gitrgrep $(DESTDIR)$(bindir)/gitrfgrep


uninstall-hook:
	-rm -f $(DESTDIR)$(bindir)/git
	-rm -f $(DESTDIR)$(bindir)/gitregrep $(DESTDIR)$(bindir)/gitrfgrep
