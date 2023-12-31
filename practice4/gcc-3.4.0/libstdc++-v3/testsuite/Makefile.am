## Makefile for the testsuite subdirectory of the GNU C++ Standard library.
##
## Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.
##
## This file is part of the libstdc++ version 3 distribution.
## Process this file with automake to produce Makefile.in.

## This file is part of the GNU ISO C++ Library.  This library is free
## software; you can redistribute it and/or modify it under the
## terms of the GNU General Public License as published by the
## Free Software Foundation; either version 2, or (at your option)
## any later version.

## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License along
## with this library; see the file COPYING.  If not, write to the Free
## Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
## USA.

AUTOMAKE_OPTIONS = dejagnu

include $(top_srcdir)/fragment.am

AM_MAKEFLAGS = -j1
AM_RUNTESTFLAGS =

## CXX is actually a "C" compiler. These are real C++ programs.
testsuite_flags_script=${glibcxx_builddir}/scripts/testsuite_flags
CXX         = $(shell ${testsuite_flags_script} --build-cxx)
AM_CXXFLAGS = $(shell ${testsuite_flags_script} --cxxflags)

GLIBGCC_DIR=`$(CC) -print-libgcc-file-name | sed 's,/[^/]*$$,,'`
GLIBCXX_DIR=${glibcxx_builddir}/src/.libs

CXXLINK = \
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) \
	-R $(GLIBGCC_DIR) -R $(GLIBCXX_DIR) \
	$(AM_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@

# Generated lists of files to run.  All of these names are valid make
# targets, if you wish to generate a list manually.
lists_of_files = \
   testsuite_files \
   testsuite_files_interactive \
   testsuite_files_performance

## Build support library.
noinst_LIBRARIES = libv3test.a
libv3test_a_SOURCES = testsuite_hooks.cc testsuite_allocator.cc

## Build support utilities.
if GLIBCXX_TEST_ABI
noinst_PROGRAMS = abi_check
else
noinst_PROGRAMS =
endif
abi_check_SOURCES = abi_check.cc

all-local: stamp_wchar testsuite_files

# Enable wchar_t tests if capable.
if GLIBCXX_TEST_WCHAR_T
stamp_wchar:
	touch testsuite_wchar_t
else
stamp_wchar:
endif

# This is automatically run after the generated check-DEJAGNU rule.
check-local: check-abi

baseline_file = ${baseline_dir}/baseline_symbols.txt
extract_symvers = $(glibcxx_srcdir)/scripts/extract_symvers

current_symbols.txt: ${extract_symvers} ../src/.libs/libstdc++.so
	  -@(${extract_symvers} ../src/.libs/libstdc++.so current_symbols.txt)

baseline_symbols:
	-@(output=${baseline_file}; \
	  if test ! -f $${output}; then \
	    echo "Baseline file doesn't exist."; \
	    echo "Try 'make new-abi-baseline' to create it."; \
	    exit 1; \
	  fi; true)

new-abi-baseline:
	-@$(mkinstalldirs) ${baseline_dir}
	-@(output=${baseline_file}; \
	  if test -f $${output}; then \
	    output=$${output}.new; \
	    t=`echo $${output} | sed 's=.*config/abi/=='`; \
	    echo "Baseline file already exists, writing to $${t} instead."; \
	  fi; \
	  ${extract_symvers} ../src/.libs/libstdc++.so $${output})

if GLIBCXX_TEST_ABI
# Use 'new-abi-baseline' to create an initial symbol file.  Then run
# 'check-abi' to test for changes against that file.
check-abi: abi_check baseline_symbols current_symbols.txt
	-@./abi_check --check ./current_symbols.txt ${baseline_file} \
	  2>&1 | tee libstdc++-abi.sum
	-@cp libstdc++-abi.sum libstdc++-abi.log

check-abi-verbose: abi_check baseline_symbols current_symbols.txt
	-@./abi_check --check-verbose ./current_symbols.txt ${baseline_file} \
	  2>&1 | tee libstdc++-abi.sum
	-@cp libstdc++-abi.sum libstdc++-abi.log

else
check-abi:
check-abi-verbose:
endif


# These two special 'check-script' rules use the bash script
# 'check_survey' to do testing. This script is not as portable as the
# dejagnu test harness, and is thus off by default. It does produce
# interesting output however, including various performance analysis
# items like compile time, execution time, and binary size.
survey_script = ${glibcxx_builddir}/scripts/check_survey
check-script: ${survey_script}
	-@(chmod + ${survey_script}; \
	  ${survey_script} 0)

check-script-install: ${survey_script}
	-@(chmod + ${survey_script}; \
	  ${survey_script} 0)


# Runs the testsuite/performance tests.
# Some of these tests create large (~75MB) files, allocate huge
# ammounts of memory, or otherwise tie up machine resources. Thus,
# running this is off by default.
performance_script=${glibcxx_srcdir}/scripts/check_performance
check-performance: testsuite_files_performance ${performance_script}
	-@(chmod + ${performance_script}; \
	  ${performance_script} ${glibcxx_srcdir} ${glibcxx_builddir})


# This rule generates all of the testsuite_files* lists at once.
${lists_of_files}:
	${glibcxx_srcdir}/scripts/create_testsuite_files \
	  ${glibcxx_srcdir}/testsuite `${PWD_COMMAND}`


.PHONY: baseline_symbols new-abi-baseline check-abi check-abi-verbose \
check-script check-script-install check-performance

# By adding these files here, automake will remove them for 'make clean'
CLEANFILES = *.txt *.tst *.exe core* filebuf_* tmp* ostream_* *.log *.sum \
	     testsuite_* site.exp abi_check baseline_symbols
