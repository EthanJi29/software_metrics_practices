// 2003-04-07 bkoz

// Copyright (C) 2003 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// 27.7.1 - Template class basic_stringbuf
// NB: This file is for testing basic_filebuf with NO OTHER INCLUDES.

#include <sstream>
#include <testsuite_hooks.h>
#include <testsuite_allocator.h>

// { dg-do compile }

namespace test 
{
  using namespace std;
  using __gnu_test::pod_char;
  typedef __gnu_test::tracker_alloc<char> alloc_type;
  template class basic_stringbuf<char, char_traits<char>, alloc_type>;
} // test

// more surf!!!









