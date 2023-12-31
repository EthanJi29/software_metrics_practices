// 2004-03-02  Petur Runolfsson  <peturr02@ru.is>

// Copyright (C) 2004 Free Software Foundation, Inc.
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

// 27.4.3 fpos

#include <iterator>
#include <testsuite_hooks.h>

// libstdc++/14320
int test01()
{
  using namespace std;
  bool test __attribute__((unused)) = true;

  typedef std::istreambuf_iterator<char>::difference_type Distance;
  Distance d = 2;
  Distance e = 3;
  d *= e;
  VERIFY( static_cast<int>(d) == 6 );
}

int main()
{
  test01();
  return 0;
}
