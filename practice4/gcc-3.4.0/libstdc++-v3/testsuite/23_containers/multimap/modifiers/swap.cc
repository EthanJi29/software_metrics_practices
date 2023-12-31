// Copyright (C) 2004 Free Software Foundation
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

#include <map>
#include <testsuite_hooks.h>
 
struct T { int i; };

int swap_calls;

namespace std
{
  template<> 
    void 
    multimap<T, int>::swap(multimap<T, int>&) 
    { ++swap_calls; }
}

// Should use multimap specialization for swap.
void test01()
{
  bool test __attribute__((unused)) = true;
  std::multimap<T, int> A;
  std::multimap<T, int> B;
  swap_calls = 0;
  std::swap(A, B);
  VERIFY(1 == swap_calls);
}

// Should use multimap specialization for swap.
void test02()
{
  bool test __attribute__((unused)) = true;
  using namespace std;
  multimap<T, int> A;
  multimap<T, int> B;
  swap_calls = 0;
  swap(A, B);
  VERIFY(1 == swap_calls);
}

#if !__GXX_WEAK__ && _MT_ALLOCATOR_H
template class __gnu_cxx::__mt_alloc<std::_Rb_tree_node<std::pair<T const, int> > >;
#endif

// See c++/13658 for background info.
int main()
{
  test01();
  test02();
  return 0;
}
