// 2000-01-10 bkoz

// Copyright (C) 2000, 2001, 2003 Free Software Foundation, Inc.
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

// 27.7.2.2 member functions (istringstream_members)

#include <sstream>
#include <testsuite_hooks.h>

void test01()
{
  bool test __attribute__((unused)) = true;
  std::istringstream is01;
  const std::string str00; 
  const std::string str01 = "123";
  std::string str02;
  const int i01 = 123;
  int a,b;

  std::ios_base::iostate state1, state2, statefail, stateeof;
  statefail = std::ios_base::failbit;
  stateeof = std::ios_base::eofbit;

  // string str() const
  str02 = is01.str();
  VERIFY( str00 == str02 );

  // void str(const basic_string&)
  is01.str(str01);
  str02 = is01.str();
  VERIFY( str01 == str02 );
  state1 = is01.rdstate();
  is01 >> a;
  state2 = is01.rdstate();
  VERIFY( a = i01 );
  // 22.2.2.1.2 num_get virtual functions
  // p 13
  // in any case, if stage 2 processing was terminated by the test for
  // in == end then err != ios_base::eofbit is performed.
  VERIFY( state1 != state2 );
  VERIFY( state2 == stateeof ); 

  is01.str(str01);
  is01 >> b;
  VERIFY( b != a ); 
  // as is01.good() is false, istream::sentry blocks extraction.

  is01.clear();
  state1 = is01.rdstate();
  is01 >> b;
  state2 = is01.rdstate();
  VERIFY( b == a ); 
  VERIFY( state1 != state2 );
  VERIFY( state2 == stateeof ); 
}

int main()
{
  test01();
  return 0;
}



