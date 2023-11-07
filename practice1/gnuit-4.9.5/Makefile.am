AUTOMAKE_OPTIONS = 1.9

EXTRA_DIST = PLATFORMS PROBLEMS LSM.in version.h.in\
	     gnuit.spec.in mkvcs prepare \
	     m4/gnulib-tool.m4 m4/gnulib-cache.m4 \
	     m4/gnulib-common.m4 m4/utimes.m4 m4/sockpfaf.m4 \
	     LSM gnuit.spec .gitignore

CONFIG_CLEAN_FILES= version.h
distcleancheck_listfiles = find -type f -exec sh -c 'test -f $(srcdir)/{} || echo {}' ';'

GIT_OBSOLETE=dpkg-help.txt .gitrc.aixterm .gitrc.ansi .gitrc.common	\
 	     .gitrc.generic .gitrc.hft .gitrc.hpterm .gitrc.linux	\
 	     .gitrc.linux-m .gitrc.mach .gitrc.minix .gitrc.thix	\
 	     .gitrc.vt100 .gitrc.vt102 .gitrc.xterm .gitrc.xterm-color	\
 	     .gitrc.Eterm .gitrc.console .gitrc.dtterm .gitrc.eterm	\
 	     .gitrc.hp .gitrc.iris-ansi .gitrc.iris-ansi-net		\
 	     .gitrc.pc3 .gitrc.rxvt .gitrc.screen .gitrc.sun		\
 	     .gitrc.sun-cmd .gitrc.vt125 .gitrc.vt200 .gitrc.vt201	\
 	     .gitrc.vt220 .gitrc.vt240 .gitrc.vt300 .gitrc.vt320	\
 	     .gitrc.vt400 .gitrc.vt420 .gitrc.xterm-debian		\
 	     .gitrc.xterms

SUBDIRS = lib src term doc

ACLOCAL_AMFLAGS = -I m4

html:
	cd doc && $(MAKE) html

dist-hook:
	cp LSM gnuit.spec $(distdir)

# clean up obsolete files, allow for locations with and without /share/
install-data-hook:
	for i in $(GIT_OBSOLETE); \
	do\
	    if [ -e "$(DESTDIR)$(datadir)/git/$${i}" ]; then\
	        rm -f "$(DESTDIR)$(datadir)/git/$${i}" ];\
	    fi;\
	done
	-rm -f $(DESTDIR)$(infodir)/git.info* >/dev/null 2>&1
	-rm -f $(DESTDIR)$(prefix)/info/git.info* >/dev/null 2>&1
	if [ -e "$(DESTDIR)$(htmldir)/git.html" ]; then \
	    rm -f "$(DESTDIR)$(htmldir)/git.html" ];\
	fi
	if [ -e "$(DESTDIR)$(prefix)/doc/git.html" ]; then \
	    rm -f "$(DESTDIR)$(prefix)/doc/git.html" ];\
	fi
	if [ -e "$(DESTDIR)$(prefix)/doc/git-4.3.20/git.html" ]; then \
	    rm -f "$(DESTDIR)$(prefix)/doc/git-4.3.20/git.html" ;\
	    rmdir "$(DESTDIR)${prefix}/doc/git-4.3.20" >/dev/null 2>&1 || true; \
	fi
	rmdir "$(DESTDIR)$(datadir)/git" >/dev/null 2>&1 || true

