// Copyright (C) 2003 Free Software Foundation
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

// 27.4.2.1 - Types [lib.ios.types]

#include <ios>
#include <testsuite_hooks.h>

void test01()
{
  std::streamsize ssize; 
  std::streamoff soff;

  // No signed/unsigned warnings.
  long l __attribute__((unused)) = ssize;

  // Wrapped in pos_type is EOF.
  soff = -1;
}

int main(void)
{
  test01();
  return 0;
}
