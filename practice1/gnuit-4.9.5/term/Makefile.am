EXTRA_DIST = gnuitrc.aixterm.in gnuitrc.ansi.in gnuitrc.common.IN\
	     gnuitrc.generic.in gnuitrc.hft.in gnuitrc.hpterm.in gnuitrc.linux.in\
	     gnuitrc.mach.in gnuitrc.minix.in gnuitrc.thix.in gnuitrc.vt100.in\
	     gnuitrc.vt102.in gnuitrc.xterm.in gnuitrc.xterm-color.in gnuitrc.linux-m.in
pkgdata_DATA = gnuitrc.aixterm gnuitrc.ansi gnuitrc.common\
	       gnuitrc.generic gnuitrc.hft gnuitrc.hpterm gnuitrc.linux\
	       gnuitrc.mach gnuitrc.minix gnuitrc.thix gnuitrc.vt100\
	       gnuitrc.vt102 gnuitrc.xterm gnuitrc.xterm-color gnuitrc.linux-m
CLEANFILES = gnuitrc.common

GIT_GENERICS	 = gnuitrc.sun gnuitrc.sun-cmd gnuitrc.eterm gnuitrc.pc3
GIT_XTERMS	 = gnuitrc.xterms
GIT_ANSIS	 = gnuitrc.iris-ansi gnuitrc.iris-ansi-net gnuitrc.cygwin
GIT_COLOR_XTERMS = gnuitrc.dtterm gnuitrc.rxvt gnuitrc.xterm-debian gnuitrc.Eterm gnuitrc.rxvt-unicode
GIT_LINUXS	 = gnuitrc.console
GIT_HPTERMS	 = gnuitrc.hp
GIT_VT100S	 = gnuitrc.vt125 gnuitrc.vt200 gnuitrc.vt201 gnuitrc.vt220\
		   gnuitrc.vt240 gnuitrc.vt300 gnuitrc.vt320 gnuitrc.vt400\
		   gnuitrc.vt420 gnuitrc.screen


gnuitrc.common: Makefile $(srcdir)/gnuitrc.common.IN
	sed -e 's,@htmldir\@,$(htmldir),g' \
	    -e 's,@VERSION\@,$(VERSION),g' < $(srcdir)/gnuitrc.common.IN > gnuitrc.common

install-data-hook:
	for i in $(GIT_XTERMS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	    ln -s $(datadir)/gnuit/gnuitrc.xterm $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_COLOR_XTERMS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	    ln -s $(datadir)/gnuit/gnuitrc.xterm-color $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_HPTERMS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	    ln -s $(datadir)/gnuit/gnuitrc.hpterm $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_GENERICS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	    ln -s $(datadir)/gnuit/gnuitrc.generic $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_VT100S);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	    ln -s $(datadir)/gnuit/gnuitrc.vt100 $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_ANSIS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	    ln -s $(datadir)/gnuit/gnuitrc.ansi $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_LINUXS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	    ln -s $(datadir)/gnuit/gnuitrc.linux $(DESTDIR)$(datadir)/gnuit/$$i;\
	done

uninstall-local:
	for i in $(GIT_XTERMS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_COLOR_XTERMS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_HPTERMS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_GENERICS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_VT100S);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_ANSIS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
	\
	for i in $(GIT_LINUXS);\
	do\
	    rm -f $(DESTDIR)$(datadir)/gnuit/$$i;\
	done
