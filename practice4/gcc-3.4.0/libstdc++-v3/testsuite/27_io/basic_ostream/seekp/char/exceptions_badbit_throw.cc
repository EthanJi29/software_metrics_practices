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

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

#include <ostream>
#include <streambuf>
#include <sstream>
#include <testsuite_hooks.h>
#include <testsuite_io.h>

using namespace std;

void test01()
{
  bool test __attribute__((unused)) = true;
  __gnu_test::fail_streambuf bib;
  ostream stream(&bib);
  stream.exceptions(ios_base::badbit);

  ostream::pos_type pos;

  try
    {
      stream.seekp(pos);
      VERIFY( false );
    }
  catch (const __gnu_test::positioning_error&) 
    {
      // stream should set badbit and rethrow facet_error.
      VERIFY( stream.bad() );
      VERIFY( (stream.rdstate() & ios_base::failbit) == 0 );
      VERIFY( !stream.eof() );
    }
  catch (...) 
    {
      VERIFY(false);
    }
}

void test02()
{
  bool test __attribute__((unused)) = true;
  __gnu_test::fail_streambuf bib;
  ostream stream(&bib);
  stream.exceptions(ios_base::badbit);

  ostream::off_type off;

  try
    {
      stream.seekp(off, ios_base::cur);
      VERIFY( false );
    }
  catch (const __gnu_test::positioning_error&) 
    {
      // stream should set badbit and rethrow facet_error.
      VERIFY( stream.bad() );
      VERIFY( (stream.rdstate() & ios_base::failbit) == 0 );
      VERIFY( !stream.eof() );
    }
  catch (...) 
    {
      VERIFY(false);
    }
}

// libstdc++/9546
int main()
{
  test01();
  test02();
  return 0;
}
